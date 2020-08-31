import contextlib
import copy
import glob
import os
import subprocess

import numpy as np

import tvm
import tvm.rpc
import tvm.micro
import tvm.relay

from tvm.micro.contrib import zephyr
from tvm.contrib import device_util
from tvm.contrib import util

BUILD = True
DEBUG = False


TARGET = tvm.target.target.micro('stm32f746xx')

def _make_sess_from_op(op_name, sched, arg_bufs):
  with tvm.transform.PassContext(opt_level=3, config={'tir.disable_vectorize': True}):
    mod = tvm.build(sched, arg_bufs, TARGET, target_host=TARGET, name=op_name)

  return _make_session(mod)


def _make_session(mod):
  workspace = tvm.micro.Workspace(debug=True)

  project_dir = os.path.join(os.path.dirname(__file__) or '.', 'zephyr-runtime')
  compiler = zephyr.ZephyrCompiler(
    project_dir=project_dir,
    board='nucleo_f746zg',
    zephyr_toolchain_variant='gnuarmemb',
    env_vars={'GNUARMEMB_TOOLCHAIN_PATH': '/usr/local'},
  )

  opts = tvm.micro.default_options(f'{project_dir}/crt')
  # TODO(weberlo) verify this is necessary
  opts['bin_opts']['ccflags'] = ['-std=gnu++14']
  opts['lib_opts']['ccflags'] = ['-std=gnu++14']

  flasher_kw = {}
  if DEBUG:
#    flasher_kw['debug_remote_hostport'] = '{}:{:d}'.format(*device_transport.openocd_gdb_host_port_tuple(0))
#    flasher_kw['debug_wrapping_context_manager'] = device_transport.launch(None)
    flasher_kw['debug_rpc_session'] = tvm.rpc.connect('127.0.0.1', 9090)

  session_kw = {
    'flasher': compiler.flasher(**flasher_kw),
  }

  prev_build = f'{os.path.splitext(__file__)[0]}-last-build.micro-binary'
  if BUILD:
    session_kw['binary'] = tvm.micro.build_static_runtime(
      # the x86 compiler *expects* you to give the exact same dictionary for both
      # lib_opts and bin_opts. so the library compiler is mutating lib_opts and
      # the binary compiler is expecting those mutations to be in bin_opts.
      # TODO(weberlo) fix this very bizarre behavior
      workspace, compiler, mod, lib_opts=opts['lib_opts'], bin_opts=opts['bin_opts'])
    if os.path.exists(prev_build):
      os.unlink(prev_build)
    session_kw['binary'].archive(prev_build)
  else:
    with util.TempDirectory.set_keep_for_debug():
      unarchive_dir = util.tempdir()
    session_kw['binary'] = tvm.micro.MicroBinary.unarchive(prev_build, unarchive_dir.relpath('binary'))

  return tvm.micro.Session(**session_kw)


def _make_add_sess():
  A = tvm.te.placeholder((2,), dtype='int8')
  B = tvm.te.placeholder((1,), dtype='int8')
  C = tvm.te.compute(A.shape, lambda i: A[i] + B[0], name='C')
  sched = tvm.te.create_schedule(C.op)
  return _make_sess_from_op('add', sched, [A, B, C])


def _make_ident_sess():
  A = tvm.te.placeholder((2,), dtype='int8')
  B = tvm.te.compute(A.shape, lambda i: A[i], name='B')
  sched = tvm.te.create_schedule(B.op)
  return _make_sess_from_op('ident', sched, [A, B])


def test_compile_runtime():
  """Test compiling the on-device runtime."""
  with _make_add_sess() as sess:
    A_data = tvm.nd.array(np.array([2, 3], dtype='int8'), ctx=sess.context)
    assert (A_data.asnumpy() == np.array([2, 3])).all()
    B_data = tvm.nd.array(np.array([4], dtype='int8'), ctx=sess.context)
    assert (B_data.asnumpy() == np.array([4])).all()
    C_data = tvm.nd.array(np.array([0, 0], dtype='int8'), ctx=sess.context)
    assert (C_data.asnumpy() == np.array([0, 0])).all()

    system_lib = sess.get_system_lib()
    system_lib.get_function('add')(A_data, B_data, C_data)
    assert (C_data.asnumpy() == np.array([6, 7])).all()


if __name__ == '__main__':
  import logging
  logging.basicConfig(level=logging.DEBUG)
  test_compile_runtime()
#  test_autotvm()
