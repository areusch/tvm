import tempfile

import os
import numpy as np
import tvm
import tvm.micro
import tvm.relay
import tvm.relay.testing

#/    %0 = cast(cast(%data, int16) - cast(%mean_data, int16), int8);
 # %2 = nn.bias_add(%1, cast(%conv0_bias, "int32"), axis=3);

RELAY_MODEL = """
#[version = "0.0.5"]
def @main(%data : Tensor[(1, 64, 64, 3), uint8], %mean_data : Tensor[(1, 64, 64, 3), int8], %conv0_weight : Tensor[(5, 5, 8, 3), int8], %conv0_bias : Tensor[(1, 32, 32, 8), int8]) {
    %1 = nn.conv2d(
         %data,
         %conv0_weight,
         padding=[2, 2],
         channels=32,
         kernel_size=[5, 5],
         out_dtype="int32");
  %3 = right_shift(%1, 9);
  %4 = cast(%3, "int8");
  %4
}
"""


def test_relay_model():
  data = tvm.relay.var("data", shape=(1, 3, 64, 64), dtype="uint8")
  weight = tvm.relay.var("weight", shape=(8, 3, 5, 5), dtype="int8")
  out = tvm.relay.nn.conv2d(data=data, weight=weight, kernel_size=5, data_layout='NCHW', kernel_layout='OIHW')
  func = tvm.relay.Function(tvm.relay.analysis.free_vars(out), out)
#  model = tvm.IRModule()
#  model["main"] = func
#  model = tvm.parser.fromtext(RELAY_MODEL)
  mod, params = tvm.relay.testing.create_workload(func)
  weight_data = np.random.random_integers(-127, 128, params['weight'].shape).astype("int8")
  params['weight'].copyfrom(weight_data)

#  print(str(mod))
  target = 'c -mcpu=native --runtime=c --system-lib'
  with tvm.transform.PassContext(opt_level=3, config={"tir.disable_vectorize": True}):
    lib, aot = tvm.relay.build(mod, target, params=params)

  ws = tvm.micro.Workspace(debug=True)
  mod_path = f'{ws.path}/lib.c'
  lib.lib.save(mod_path, 'cc')
  with open(mod_path, 'a+') as f:
    f.write(aot)

  print('------------------- Graph -------------------')
  print(lib.graph_json)
  print('-------------------- AOT --------------------')
  print(aot)

  compiler = tvm.micro.DefaultCompiler(target)
  opts = tvm.micro.default_options(os.path.join(tvm.micro.CRT_ROOT_DIR, "host"))
  micro_bin = tvm.micro.build_static_runtime(ws, compiler, mod_path, opts['lib_opts'], opts['bin_opts'])
  with tvm.micro.Session(binary=micro_bin, flasher=compiler.flasher(debug=False)) as sess:
    mod = sess.get_system_lib()
    main = mod.get_function('main_func')
    A_data = np.random.random_integers(0, 255, [1, 64, 64, 3]).astype("uint8")
    A = tvm.nd.array(A_data, ctx=sess.context)
    B = tvm.nd.array(np.zeros([1, 8, 60, 60], dtype="uint8"), ctx=sess.context)
    main(A, B)
    aot_output = B.asnumpy()


  with tvm.micro.Session(binary=micro_bin, flasher=compiler.flasher(debug=False)) as sess:
    graph_mod = tvm.micro.create_local_graph_runtime(
      lib.get_json(), sess.get_system_lib(), sess.context
    )

    graph_mod.set_input(**lib.params)
    graph_mod.run(data=tvm.nd.array(A_data, ctx=sess.context))
    graph_output = graph_mod.get_output(0).asnumpy()

  np.testing.assert_allclose(aot_output, graph_output)
  print("ZOMG")


test_relay_model()
