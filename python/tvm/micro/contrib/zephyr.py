import multiprocessing
import os
import shutil
import subprocess
import sys

import tvm.micro
from . import base


class ZephyrCompiler(tvm.micro.Compiler):

  def __init__(self, project_dir=None, board=None, west_cmd=None, west_build_args=None,
               zephyr_toolchain_variant=None, env_vars=None):
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
    self._zephyr_toolchain_variant = zephyr_toolchain_variant
    self._env_vars = env_vars

  def _invoke_in_env(self, cmd, **kw):
    env = dict(os.environ)
    if self._zephyr_toolchain_variant is not None:
      env['ZEPHYR_TOOLCHAIN_VARIANT'] = self._zephyr_toolchain_variant

    if self._env_vars is not None:
      for k, v in self._env_vars.items():
        env[k] = v

    return subprocess.check_output(cmd, env=env, **kw)

  OPT_KEY_TO_CMAKE_DEFINE = {
    'cflags': 'CFLAGS',
    'ccflags': 'CXXFLAGS',
    'ldflags': 'LDFLAGS',
  }

  @classmethod
  def _OptionsToCmakeArgs(cls, options):
    args = []
    for key, define in cls.OPT_KEY_TO_CMAKE_DEFINE.items():
      if key in options:
        args.append(f'-DEXTRA_{define}={";".join(options[key])}')

    if 'cmake_args' in options:
      args.extend(options['cmake_args'])

    return args

  def Library(self, output, objects, options=None):
    project_name = os.path.basename(output)
    if project_name.startswith('lib'):
      project_name = project_name[3:]

    with open(os.path.join(output, 'prj.conf'), 'w') as prj_conf_f:
      prj_conf_f.write(
        'CONFIG_CPLUSPLUS=y\n'
#        'CONFIG_NEWLIB_LIBC=y\n'
#        'CONFIG_LIB_CPLUSPLUS=y\n'
      )

    cmakelists_path = os.path.join(output, 'CMakeLists.txt')
    with open(cmakelists_path, 'w') as cmake_f:
      sources = ' '.join(f'"{o}"' for o in objects)
      cmake_f.write(
        'cmake_minimum_required(VERSION 3.13.1)\n'
        '\n'
        'find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})\n'
        f'project({project_name}_prj)\n'
        f'target_sources(app PRIVATE)\n'
        f'zephyr_library_named({project_name})\n'
        f'target_sources({project_name} PRIVATE {sources})\n'
        f'target_sources(app PRIVATE main.c)\n'
        f'target_link_libraries(app PUBLIC {project_name})\n'
#        f'target_link_libraries({project_name} zephyr_interface)\n'
      )
      if 'include_dirs' in options:
        cmake_f.write(f'target_include_directories({project_name} PRIVATE {" ".join(options["include_dirs"])})\n')


    with open(os.path.join(output, 'main.c'), 'w'):
      pass

    # expecetd not to exist after populate_tvm_libs
    build_dir = os.path.join(output, '__tvm_build')
    print('lib build dir', build_dir)
    os.mkdir(build_dir)
    self._invoke_in_env(['cmake',  '..', f'-DBOARD={self._board}'] +
                        self._OptionsToCmakeArgs(options), cwd=build_dir)
    num_cpus = multiprocessing.cpu_count()
    self._invoke_in_env(['make', f'-j{num_cpus}', 'VERBOSE=1', project_name], cwd=build_dir)
    return tvm.micro.MicroLibrary(build_dir, [f'lib{project_name}.a'])

  def Binary(self, output, objects, options=None):
    print('generating in', self._project_dir)
    assert self._project_dir is not None, (
      'Must supply project_dir= to build binaries')

    copied_libs = base.populate_tvm_objs(self._project_dir, objects)

    # expecetd not to exist after populate_tvm_objs
    build_dir = os.path.join(self._project_dir, '__tvm_build')
    os.mkdir(build_dir)
    cmake_args = ['cmake', '..', f'-DBOARD={self._board}'] + self._OptionsToCmakeArgs(options)
    if 'include_dirs' in options:
      cmake_args.append(f'-DTVM_INCLUDE_DIRS={";".join(options["include_dirs"])}')
    cmake_args.append(f'-DTVM_LIBS={";".join(copied_libs)}')
    print('cmake', cmake_args)
    self._invoke_in_env(cmake_args, cwd=build_dir)

    self._invoke_in_env(['make'], cwd=build_dir)

    to_copy = ['zephyr.elf', 'zephyr.bin']
    for f in to_copy:
      shutil.copy(os.path.join(build_dir, 'zephyr', f), os.path.join(output, f))
