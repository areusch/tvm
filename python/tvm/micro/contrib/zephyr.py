import collections
import multiprocessing
import os
import re
import termios
import textwrap
import signal
import shlex
import shutil
import subprocess
import sys

import yaml

import tvm.micro
from . import base
from .. import compiler
from .. import debugger
from ..transport import debug
from ..transport import serial


class SubprocessEnv(object):

    def __init__(self, default_overrides):
      self.default_overrides = default_overrides

    def run(self, cmd, **kw):
        env = dict(os.environ)
        for k, v in self.default_overrides.items():
            env[k] = v

        return subprocess.check_output(cmd, env=env, **kw)


class FlashRunnerNotSupported(Exception):
    """Raised when the FLASH_RUNNER for a project isn't supported by this Zephyr adapter."""


class ZephyrCompiler(tvm.micro.Compiler):

    def __init__(self, project_dir=None, board=None, west_cmd=None, west_build_args=None,
                 zephyr_base=None, zephyr_toolchain_variant=None, env_vars=None):
        self._project_dir = project_dir
        self._board = board
        if west_cmd is None:
            self._west_cmd = [sys.executable, '-mwest.app.main']
        elif isinstance(west_cmd, str):
            self._west_cmd = [west_cmd]
        elif isinstance(west_cmd, list):
            self._west_cmd = west_cmd
        else:
            raise TypeError('west_cmd: expected string, list, or None; got %r' % (west_cmd,))

        self._west_build_args = west_build_args
        env = {}
        if zephyr_toolchain_variant is not None:
            env['ZEPHYR_TOOLCHAIN_VARIANT'] = zephyr_toolchain_variant

        self._zephyr_base = zephyr_base or os.environ['ZEPHYR_BASE']
        assert self._zephyr_base is not None, (
            f'Must specify zephyr_base=, or ZEPHYR_BASE must be in environment variables')
        env['ZEPHYR_BASE'] = self._zephyr_base

        if env_vars:
            env.update(env_vars)

        self._subprocess_env = SubprocessEnv(env)

    OPT_KEY_TO_CMAKE_DEFINE = {
        'cflags': 'CFLAGS',
        'ccflags': 'CXXFLAGS',
        'ldflags': 'LDFLAGS',
    }

    @classmethod
    def _options_to_cmake_args(cls, options):
        args = []
        for key, define in cls.OPT_KEY_TO_CMAKE_DEFINE.items():
            if key in options:
                args.append(f'-DEXTRA_{define}={";".join(options[key])}')

        if 'cmake_args' in options:
            args.extend(options['cmake_args'])

        return args

    def library(self, output, objects, options=None):
        project_name = os.path.basename(output)
        if project_name.startswith('lib'):
            project_name = project_name[3:]

        lib_prj_conf = os.path.join(output, 'prj.conf')
        if self._project_dir is not None:
            project_dir_conf = os.path.join(self._project_dir, 'prj.conf')
            if os.path.exists(project_dir_conf):
                shutil.copy(project_dir_conf, lib_prj_conf)
        else:
            with open(lib_prj_conf, 'w') as prj_conf_f:
                prj_conf_f.write(
                  'CONFIG_CPLUSPLUS=y\n'
                )

        cmakelists_path = os.path.join(output, 'CMakeLists.txt')
        with open(cmakelists_path, 'w') as cmake_f:
            sources = ' '.join(f'"{o}"' for o in objects)
            cmake_f.write(textwrap.dedent(f"""\
                cmake_minimum_required(VERSION 3.13.1)

                find_package(Zephyr HINTS $ENV{{ZEPHYR_BASE}})
                project({project_name}_prj)
                target_sources(app PRIVATE)
                zephyr_library_named({project_name})
                target_sources({project_name} PRIVATE {sources})
                target_sources(app PRIVATE main.c)
                target_link_libraries(app PUBLIC {project_name})
                """))
            if 'include_dirs' in options:
                cmake_f.write(
                    f'target_include_directories({project_name} PRIVATE '
                    f'{" ".join(os.path.abspath(d) for d in options["include_dirs"])})\n')


        with open(os.path.join(output, 'main.c'), 'w'):
            pass

        # expecetd not to exist after populate_tvm_libs
        build_dir = os.path.join(output, '__tvm_build')
        print('lib build dir', build_dir)
        os.mkdir(build_dir)
        self._subprocess_env.run(['cmake',  '..', f'-DBOARD={self._board}'] +
                                 self._options_to_cmake_args(options), cwd=build_dir)
        num_cpus = multiprocessing.cpu_count()
        self._subprocess_env.run(['make', f'-j{num_cpus}', 'VERBOSE=1', project_name], cwd=build_dir)
        return tvm.micro.MicroLibrary(build_dir, [f'lib{project_name}.a'])

    def binary(self, output, objects, options=None):
        print('generating in', self._project_dir)
        assert self._project_dir is not None, (
            'Must supply project_dir= to build binaries')

        copied_libs = base.populate_tvm_objs(self._project_dir, objects)

        # expected not to exist after populate_tvm_objs
        build_dir = os.path.join(self._project_dir, '__tvm_build')
        os.mkdir(build_dir)
#        build_dir = output
        cmake_args = ['cmake', '..', f'-DBOARD={self._board}'] + self._options_to_cmake_args(options)
        if 'include_dirs' in options:
            cmake_args.append(
                f'-DTVM_INCLUDE_DIRS={";".join(os.path.abspath(d) for d in options["include_dirs"])}')
        cmake_args.append(f'-DTVM_LIBS={";".join(copied_libs)}')
        print('cmake', cmake_args)
        self._subprocess_env.run(cmake_args, cwd=build_dir)

        self._subprocess_env.run(['make'], cwd=build_dir)

        # TODO: move this logic into MicroBinary.
        to_copy = [
            os.path.join('zephyr', 'zephyr.elf'),
            os.path.join('zephyr', 'zephyr.bin'),
            os.path.join('zephyr', 'zephyr.hex'),
            os.path.join('zephyr', 'zephyr.dts'),
            os.path.join('zephyr', 'runners.yaml'),
            'CMakeCache.txt',
        ]
        for f in to_copy:
            dest_f = os.path.join(output, f)
            dest_dir = os.path.dirname(dest_f)
            if not os.path.exists(dest_dir):
                os.makedirs(dest_dir)
            shutil.copy(os.path.join(build_dir, f), dest_f)

        return tvm.micro.MicroBinary(output,
                                     binary_file=os.path.join('zephyr', 'zephyr.bin'),
                                     debug_files=[os.path.join('zephyr', 'zephyr.elf')],
                                     labelled_files={'cmake_cache': ['CMakeCache.txt'],
                                                     'hex_file': [os.path.join('zephyr', 'zephyr.hex')],
                                                     'runners_yaml': [os.path.join('zephyr', 'runners.yaml')],
                                                     'device_tree': [os.path.join('zephyr', 'zephyr.dts')]})

    @property
    def flasher_factory(self):
        return compiler.FlasherFactory(
            ZephyrFlasher,
            (self._west_cmd,),
            dict(
                zephyr_base=self._zephyr_base,
                project_dir=self._project_dir,
                subprocess_env=self._subprocess_env.default_overrides))


CACHE_ENTRY_RE = re.compile(r'(?P<name>[^:]+):(?P<type>[^=]+)=(?P<value>.*)')


CMAKE_BOOL_MAP = dict(
    [(k, True) for k in ('1', 'ON', 'YES', 'TRUE', 'Y')] +
    [(k, False) for k in ('0', 'OFF', 'NO', 'FALSE', 'N', 'IGNORE', 'NOTFOUND', '')])


def read_cmake_cache(file_name):
    entries = collections.OrderedDict()
    with open(file_name, encoding='utf-8') as f:
        for line in f:
            m = CACHE_ENTRY_RE.match(line.rstrip('\n'))
            if not m:
                continue

            if m.group('type') == 'BOOL':
                value = CMAKE_BOOL_MAP[m.group('value').upper()]
            else:
                value = m.group('value')

            entries[m.group('name')] = value

    return entries


class BoardError(Exception):
    pass


class BoardAutodetectFailed(Exception):
  """Raised when no attached hardware could be found matching the board= given to ZephyrCompiler."""


class ZephyrFlasher(tvm.micro.compiler.Flasher):

    def __init__(self, west_cmd, zephyr_base=None, project_dir=None, subprocess_env=None,
                 nrfjprog_snr=None, openocd_serial=None, flash_args=None, debug_rpc_session=None):
        zephyr_base = zephyr_base or os.environ['ZEPHYR_BASE']
        sys.path.insert(0, os.path.join(zephyr_base, 'scripts', 'dts'))
        try:
            import dtlib
            self._dtlib = dtlib
        finally:
            sys.path.pop(0)

        self._zephyr_base = zephyr_base
        self._project_dir = project_dir
        self._west_cmd = west_cmd
        self._flash_args = flash_args
        self._openocd_serial = openocd_serial
        self._autodetected_openocd_serial = None
        self._subprocess_env = SubprocessEnv(subprocess_env)
        self._debug_rpc_session = debug_rpc_session
        self._nrfjprog_snr = nrfjprog_snr

    def _get_nrf_device_args(self):
        nrfjprog_args = ['nrfjprog', '--ids']
        nrfjprog_ids = subprocess.check_output(nrfjprog_args, encoding='utf-8')
        if not nrfjprog_ids.strip('\n'):
            raise BoardError(f'No attached boards recognized by {" ".join(nrfjprog_args)}')

        boards = nrfjprog_ids.split('\n')[:-1]
        if len(boards) > 1:
            if self._nrfjprog_snr is None:
                raise BoardError(
                    f'Multiple boards connected; specify one with nrfjprog_snr=: {", ".join(boards)}')
            elif str(self._nrfjprog_snr) not in boards:
                raise BoardError(
                    f'nrfjprog_snr ({self._nrfjprog_snr}) not found in {nrfjprog_args}: {boards}')

            return ['--snr', str(self._nrfjprog_snr)]

        if not boards:
            return []

        return ['--snr', boards[0]]

    # kwargs passed to usb.core.find to find attached boards for the openocd flash runner.
    BOARD_USB_FIND_KW = {
        'nucleo_f746zg': {'idVendor': 0x0483, 'idProduct': 0x374b},
    }

    def openocd_serial(self, cmake_entries):
        if self._openocd_serial is not None:
            return self._openocd_serial

        elif self._autodetected_openocd_serial is None:
            import usb
            find_kw = self.BOARD_USB_FIND_KW[cmake_entries['BOARD']]
            boards = usb.core.find(find_all=True, **find_kw)
            serials = []
            for b in boards:
              serials.append(b.serial_number)

            if len(serials) == 0:
                raise BoardAutodetectFailed(f'No attached USB devices matching: {find_kw!r}')
            serials.sort()

            self._autodetected_openocd_serial = serials[0]
            print('autodetected', serials[0])

        return self._autodetected_openocd_serial


    def _get_openocd_device_args(self, cmake_entries):
        return ['--serial', self.openocd_serial(cmake_entries)]

    @classmethod
    def _get_flash_runner(cls, cmake_entries):
        flash_runner = cmake_entries.get('ZEPHYR_BOARD_FLASH_RUNNER')
        if flash_runner is not None:
            return flash_runner

        with open(cmake_entries['ZEPHYR_RUNNERS_YAML']) as f:
            doc = yaml.load(f)
        return doc['flash-runner']

    def _get_device_args(self, cmake_entries):
        flash_runner = self._get_flash_runner(cmake_entries)

        if flash_runner == 'nrfjprog':
            return self._get_nrf_device_args()
        elif flash_runner == 'openocd':
            return self._get_openocd_device_args(cmake_entries)
        else:
            raise BoardError(
                f"Don't know how to find serial terminal for board {cmake_entries['BOARD']} with flash "
                f"runner {flash_runner}")

    def flash(self, micro_binary):
        cmake_entries = read_cmake_cache(micro_binary.abspath(micro_binary.labelled_files['cmake_cache'][0]))

        build_dir = os.path.dirname(micro_binary.abspath(micro_binary.labelled_files['cmake_cache'][0]))
        west_args = self._west_cmd + ['flash',
                                      '--build-dir', build_dir,
                                      '--skip-rebuild'] + self._get_device_args(cmake_entries)
        if self._flash_args is not None:
            west_args.extend(self._flash_args)
        self._subprocess_env.run(west_args, cwd=build_dir)

        return self.transport(micro_binary)

    def _find_nrf_serial_port(self, cmake_entries):
        com_ports = subprocess.check_output(['nrfjprog', '--com'] + self._get_device_args(cmake_entries), encoding='utf-8')
        ports_by_vcom = {}
        for line in com_ports.split('\n')[:-1]:
            parts = line.split()
            ports_by_vcom[parts[2]] = parts[1]

        return {'port_path': ports_by_vcom['VCOM2']}

    def _find_openocd_serial_port(self, cmake_entries):
        return {'grep': self.openocd_serial(cmake_entries)}

    def _find_serial_port(self, micro_binary):
        cmake_entries = read_cmake_cache(micro_binary.abspath(micro_binary.labelled_files['cmake_cache'][0]))
        flash_runner = self._get_flash_runner(cmake_entries)

        if flash_runner == 'nrfjprog':
            return self._find_nrf_serial_port(cmake_entries)
        elif flash_runner == 'openocd':
            return self._find_openocd_serial_port(cmake_entries)

        raise FlashRunnerNotSupported(
            f"Don't know how to deduce serial port for flash runner {flash_runner}")

    def transport(self, micro_binary):
        dt = self._dtlib.DT(micro_binary.abspath(micro_binary.labelled_files['device_tree'][0]))
        uart_baud = dt.get_node('/chosen').props['zephyr,console'].to_path().props['current-speed'].to_num()
        print('uart baud!', uart_baud)

        port_kwargs = self._find_serial_port(micro_binary)
        print('port', port_kwargs)
        serial_transport = serial.SerialTransport(baudrate=uart_baud, **port_kwargs)
        if self._debug_rpc_session is None:
            return serial_transport

        return debug.DebugWrapperTransport(
            debugger.RpcDebugger(
                self._debug_rpc_session,
                debugger.DebuggerFactory(
                    ZephyrDebugger,
                    (' '.join(shlex.quote(x) for x in self._west_cmd),
                     os.path.join(self._project_dir, '__tvm_build'),
                     micro_binary.abspath(micro_binary.debug_files[0]),
                     self._zephyr_base),
                     {})),
                serial_transport)



class ZephyrDebugger(debugger.Debugger):

    def __init__(self, west_cmd, build_dir, elf_path, zephyr_base):
        self._west_cmd = shlex.split(west_cmd)
        self._build_dir = build_dir
        self._elf_path = elf_path
        self._zephyr_base = zephyr_base

    def start(self):
        env = dict(os.environ)
        env['ZEPHYR_BASE'] = self._zephyr_base
        sys.stdin = open(0)
        self._old_termios = termios.tcgetattr(sys.stdin)
        self._proc = subprocess.Popen(
            self._west_cmd + ['debug',
                              '--skip-rebuild',
                              '--build-dir', self._build_dir,
                              '--elf-file', self._elf_path],
            env=env)
        self._old_sigint_handler = signal.signal(signal.SIGINT, signal.SIG_IGN)

    def stop(self):
        signal.signal(signal.SIGINT, self._old_sigint_handler)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self._old_termios)
        self._proc.terminate()
        self._proc.wait()
