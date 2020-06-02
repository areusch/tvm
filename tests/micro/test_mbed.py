import glob
import os
import tvm.micro
from tvm.micro.contrib import mbed
from tvm.contrib import util

def test_compile_runtime():
  """Test compiling the on-device runtime."""
  project_dir = '/Users/andrew/ws/stm-nucleo/test'
  compiler = mbed.MbedCompiler(
    project_dir=project_dir,
    mbed_target='NUCLEO_F746ZG',
    mbed_toolchain='GCC_ARM')

  temp = util.tempdir()

  root_dir = os.path.realpath(f'{os.path.dirname(__file__)}/../..')
  srcs = glob.glob(f'{root_dir}/src/runtime/crt/*.c')
  compiler.Library(
    temp.relpath('runtime.o'),
    srcs,
    options={
      'profile': {'common': ['-Werror']},
      'args': ['--source', f'{root_dir}/include',
               '--source', f'{root_dir}/3rdparty/dlpack/include',
               '--source', f'{project_dir}/crt',
               '--source', f'{root_dir}/3rdparty/dmlc-core/include']})


if __name__ == '__main__':
  test_compile_runtime()
