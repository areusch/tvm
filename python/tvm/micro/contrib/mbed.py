import glob
import json
import logging
import os
import re
import shutil
import sys

from tvm.contrib import util
import tvm.micro
import subprocess

import mbed_os_tools.detect
import mbed_os_tools.test.host_tests_toolbox


logger = logging.getLogger(__name__)


class InvalidMbedProjectError(Exception):
  """Raised when the specified mBED project or bootstrap url is invalid."""


class MissingBuildConfigError(Exception):
  """Raised when insufficient build configuration is given to MbedCompiler."""


class BuildError(Exception):
  """Raised when there is an error performing the build."""

class NoSuchDeviceError(Exception):
  """Raised when there is no target with the given serial number."""


MBED_DEFAULT_OPTIONS = {
  'ccflags': ['-std=gnu++14'],  # Need gnu++14 for https://github.com/ARMmbed/mbed-os/issues/10548
  'ldflags': ['-std=gnu++14'],
}


class MbedCompiler(tvm.micro.Compiler):

  def _SetIfNeeded(self, mbed_config_name, value):
    if value is None:
      try:
        value = self._invoke_mbed(['config', mbed_config_name]).strip()
        if value.startswith('[mbed] '):
          value = self._mbed_target[len('[mbed] '):]

        return value

      except subprocess.CalledProcessError as e:
        raise MissingBuildConfig(
          f'mBED project does not define config {mbed_config_name}, and none is given')
    else:
      self._invoke_mbed(['config', mbed_config_name, value])
      return value

  def __init__(self, bootstrap_url=None, project_dir=None, mbed_tool_entrypoint=None,
               mbed_target=None, mbed_toolchain=None, compiler_path=None, debug=False):
    self._mbed_tool_entrypoint = mbed_tool_entrypoint
    if self._mbed_tool_entrypoint is None:
      self._mbed_tool_entrypoint = [sys.executable, '-mmbed']

    self._project_dir = project_dir
    if self._project_dir is None:
      self._project_dir = os.path.realpath(util.tempdir().temp_dir)

    if bootstrap_url is not None:
      if os.path.exists(os.path.join(self._project_dir), '.git'):
        remote = subprocess.check_output(
          ['git', 'remote', 'get-url', 'origin'],
          cwd=self._project_dir)
        if remote != bootstrap_url:
          raise InvalidMbedProjectError(
            f'mBED project in {project_url} has git remote "origin" at {remote}; expected '
            f'{bootstrap_url}')

      else:
        try:
          self._invoke_mbed(['import', bootstrap_url, '.'])
        except subprocess.CalledProcessError as e:
          raise InvalidMbedProjectError(
            f'Could not import project from url {bootstrap_url}')

    if not os.path.exists(os.path.join(self._project_dir, 'mbed_app.json')):
      raise InvalidMbedProjectError(
        f'Did not find mbed_app.json at the root of project {project_dir}')

    self._mbed_target = self._SetIfNeeded('TARGET', mbed_target)
    self._mbed_toolchain = self._SetIfNeeded('TOOLCHAIN', mbed_toolchain)

    if compiler_path is not None:
      if mbed_toolchain is None:
        raise MissingBuildConfig(
          f'compiler_path= is only respected when mbed_toolchain= is also given')

      if compiler_path is None:
        self._invoke_mbed(['config', f'{mbed_toolchain}_PATH'])

    self.debug = debug

  SOURCE_EXTS = ['s', 'c', 'cc', 'cpp', 'o', 'a']

  @classmethod
  def _CanCompileInPlace(cls, objects):
    # Prefer to point mBED compiler at the original library source. This eliminates the need to
    # make a tempdir, which means that source files referenced in the generated library will
    # continue to resolve after the python script exits.
    all_basenames = [os.path.basename(obj) for obj in objects]
    dir_files = os.listdir(os.path.dirname(objects[0]))
    for f in dir_files:
      if f in all_basenames:
        continue

      ext = os.path.splitext(f)[1]
      if ext in cls.SOURCE_EXTS:
        return False

    return True

  OPTS_TO_PROFILE_KEYS = {
      'cflags': 'c',
      'ccflags': 'cxx',
      'ldflags': 'ld',
  }

  def _OptionsToArgs(self, options):
    profile_name = options.get('profile-name', 'release.json' if not self.debug else 'develop.json')
    profile_path = os.path.join(f'{self._project_dir}/mbed-os/tools/profiles', profile_name)
    if 'profile' in options or any(k in options for k in self.OPTS_TO_PROFILE_KEYS):
      with open(profile_path) as profile_f:
        profile = json.load(profile_f)

      profile_extensions = options.get('profile', {})
      for key, opts in profile_extensions.items():
        profile[self._mbed_toolchain][key] = profile[self._mbed_toolchain].get(key, []) + opts

      for opt_key, profile_key in self.OPTS_TO_PROFILE_KEYS.items():
        if opt_key not in options:
          continue

        profile[self._mbed_toolchain][profile_key] = (
          profile[self._mbed_toolchain].get(profile_key, []) + options[opt_key])

      profile_path = os.path.join(self._project_dir, 'tvm_profile.json')
      with open(profile_path, 'w') as profile_f:
        json.dump(profile, profile_f, sort_keys=True, indent=4)

    args = ['--profile', profile_path]
    if 'include_dirs' in options:
      for inc_dir in options['include_dirs']:
        # TODO warn about stray source files.
        args.append('--source')
        args.append(inc_dir)

    return args + options.get('args', [])

  def Library(self, output, objects, options=None):
    all_dirnames = [os.path.dirname(obj) for obj in objects]
    src_dir = all_dirnames[0]
    assert all(x == src_dir for x in all_dirnames[1:]), (
      'mBED compiler wants libraries built only from files in a single directory')

    args = ['compile', '--library', '--source', src_dir]
    artifact_name = os.path.basename(output)
    args.extend(['--build', output, '-N', artifact_name])
    if options:
      args += self._OptionsToArgs(options)

    self._invoke_mbed(args)
    return tvm.micro.MicroLibrary(output, [f'lib{artifact_name}.a'])

  IMAGE_RE = re.compile('^Image: ./(.*)$', re.MULTILINE)

  GLOB_PATTERNS = ['__tvm_*', 'libtvm__*']

  def Binary(self, output, objects, options=None):
    copied = []
    for p in self.GLOB_PATTERNS:
      for f in glob.glob(os.path.join(self._project_dir, p)):
        os.unlink(f)

    for obj in objects:
      for lib_file in obj.library_files:
        obj_base = os.path.basename(lib_file)
        if obj_base.endswith('.a'):
          dest = os.path.join(self._project_dir, f'libtvm__{obj_base}')
        else:
          dest = os.path.join(self._project_dir, f'__tvm_{obj_base}')

        shutil.copy(obj.abspath(lib_file), dest)

    args = ['compile', '--source', '.']
    if options:
      args += self._OptionsToArgs(options)

    artifact_name = os.path.basename(output)
    args += ['-N', artifact_name]

    args += ['--build', output]
    self._invoke_mbed(args)

    return tvm.micro.MicroBinary(
      output, f'{artifact_name}.bin', debug_files=[f'{artifact_name}.elf'])

  def Flasher(self, **kw):
    return Flasher(self._mbed_target, **kw)

  def _invoke_mbed(self, args):
    try:
      return subprocess.check_output(self._mbed_tool_entrypoint + args,
                                     stderr=subprocess.STDOUT, cwd=self._project_dir)
    except subprocess.CalledProcessError as e:
      logger.error('%s: process exited with code %d. Stdio: \n%s',
                   ' '.join(e.cmd), e.returncode, str(e.output, 'utf-8'))
      raise e


class Flasher(tvm.micro.Flasher):

  def __init__(self, mbed_target, target_serial_number=None, debug=False, debug_remote_hostport=None,
               debug_gdb_binary='arm-none-eabi-gdb', debug_wrapping_context_manager=None):
    self._mbed_target = mbed_target
    self._target_serial_number = target_serial_number
    self._debug = debug
    self._debug_remote_hostport = debug_remote_hostport
    self._debug_gdb_binary = debug_gdb_binary
    self._debug_wrapping_context_manager = debug_wrapping_context_manager
    if debug and not debug_remote_hostport:
      raise ValueError('Must supply debug_remote_hostport= when debug=True')

    d = mbed_os_tools.detect.create()
    self._devices = d.list_mbeds(filter_function=self._filter_mbed)
    logger.debug('Found devices: %r', self._devices)
    if self._target_serial_number is not None and len(self._devices) != 1:
        raise NoSuchDeviceError(
            f'No mBED device with serial number {target_serial_number}')
    elif self._target_serial_number is None and not self._devices:
        raise NoSuchDeviceError('No mBED device found')

  def _filter_mbed(self, device):
    if device['platform_name'] != self._mbed_target:
      return False

    if self._target_serial_number is not None:
      return device['target_id_usb_id'] == target_serial_number

    return True

  def Flash(self, micro_binary):
    bin_file = micro_binary.abspath(micro_binary.binary_file)
    print('flashing', bin_file)
    mbed_os_tools.test.host_tests_toolbox.flash_dev(
        self._devices[0]['mount_point'], bin_file, program_cycle_s=4)

    serial_transport = tvm.micro.SerialTransport(port_path=self._devices[0]['serial_port'], baudrate=115200)
    if self._debug:
      return tvm.micro.DebugWrapperTransport(
        debugger=tvm.micro.GdbRemoteDebugger(self._debug_gdb_binary, self._debug_remote_hostport,
                                             micro_binary.abspath(micro_binary.debug_files[0]),
                                             wrapping_context_manager=self._debug_wrapping_context_manager),
        transport=serial_transport)

    return serial_transport
