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
  opts = {
    'profile': {'common': ['-Werror']},
    'args': ['--source', f'{root_dir}/include',
             '--source', f'{root_dir}/3rdparty/dlpack/include',
             '--source', f'{project_dir}/crt',
             '--source', f'{root_dir}/3rdparty/dmlc-core/include'],
  }

  crt_common = temp.relpath('crt_common.a')
  compiler.Library(
    crt_common,
    glob.glob(f'{root_dir}/src/runtime/crt/common/*.c'),
    options=opts)
  rpc_server = temp.relpath('rpc_server.a')
  compiler.Library(
    rpc_server,
    glob.glob(f'{root_dir}/src/runtime/crt/rpc_server/*.cc'),
    options=opts)
  print('compiled', rpc_server)

  opts['profile']['common'] = []
  binary = temp.relpath('runtime')
  compiler.Binary(
    binary,
    [crt_common, rpc_server],
    options=opts)

  compiler.Flasher().Flash(binary)

if __name__ == '__main__':
  test_compile_runtime()
