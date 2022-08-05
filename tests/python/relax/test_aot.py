# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
from __future__ import annotations  # must import to defer parsing of annotations
import os

import numpy as np
import pytest
import tvm
import tvm.script
import tvm.testing
from tvm import relax, rpc, te, tir, topi, TVMError
from tvm.contrib import utils
from tvm.relax.testing import nn
from tvm.script import relax as R, tir as T


#@tvm.register_func("test.vm.move")
#def move(src):
#    return src


#@tvm.register_func("test.vm.add")
# def add(a, b):
#     ret = a.numpy() + b.numpy()
#     return tvm.nd.array(ret)


# @tvm.register_func("test.vm.mul")
# def mul(a, b):
#     ret = a.numpy() * b.numpy()
#     return tvm.nd.array(ret)


# @tvm.register_func("test.vm.equal_zero")
# def equal_zero(a):
#     ret = np.all((a.numpy() == 0))
#     return bool(ret)


# @tvm.register_func("test.vm.subtract_one")
# def subtract_one(a):
#     ret = np.subtract(a.numpy(), 1)
#     return tvm.nd.array(ret)


# @tvm.register_func("test.vm.identity")
# def identity_packed(a, b):
#     b[:] = tvm.nd.array(a.numpy())


# @tvm.register_func("test.vm.tile")
# def tile_packed(a, b):
#     b[:] = tvm.nd.array(np.tile(a.numpy(), (1, 2)))


def test_vm_execute():
    @tvm.script.ir_module
    class TestVMMove:
        @T.prim_func
        def add_one(x: T.handle, y: T.handle) -> None:
            T.func_attr({"global_symbol": "add_one"})  # TODO: automatically add this.
            m = T.var("int32")
            n = T.var("int32")
            A = T.match_buffer(x, (m, n), dtype="float32")
            B = T.match_buffer(x, (m, n), dtype="float32")
            for i, j in T.grid(m, n):
                with T.block("add_one"):
                    B[i, j] = A[i, j] + 1.0

        @R.function
        def main(x: Tensor((3, 4), "float32")) -> Tensor:
            y = R.call_tir("add_one", x, (3, 4), dtype="float32")
            return y

    mod = TestVMMove
    target = tvm.target.Target("llvm", host="llvm")
    print("EX", mod)
    ex = relax.vm.build(mod, target, backend="aot")
    # aot = relax.build(
    # vm = relax.VirtualMachine(ex, tvm.cpu())
    # a = tvm.nd.array(
    #     np.random.rand(
    #         4,
    #     )
    # )
    # b = tvm.nd.array(
    #     np.random.rand(
    #         4,
    #     )
    # )
    # add_res = vm["func0"](a, b)
    # tvm.testing.assert_allclose(add_res.numpy(), a.numpy() + b.numpy(), rtol=1e-7, atol=1e-7)


if __name__ == "__main__":
    tvm.testing.main()
