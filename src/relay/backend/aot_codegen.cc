/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file relay/backend/aot_codegen.cc
 * \brief Graph runtime codegen
 */
#include <tvm/ir/module.h>

namespace tvm {
namespace runtime {

/*! \brief Code generator for graph runtime */
class AotRuntimeCodegen : public backend::MemoizedExprTranslator<std::vector<GraphNodeRef>> {
 public:
  GraphRuntimeCodegen(runtime::Module* mod, const TargetsMap& targets) : mod_(mod) {
    compile_engine_ = CompileEngine::Global();
    targets_ = targets;
  }

  void Codegen() {
    auto pf = GetPackedFunc("relay.backend.GraphPlanMemory");
    storage_device_map_ = (*pf)(func);
    heads_ = VisitExpr(func->body);
  }
};

class AotRuntimeCodegenModule : public runtime::ModuleNode {
 public:
  virtual PackedFunc GetFunction(const std::string& name, const ObjectPtr<Object>& sptr_to_self) {
    if (name == "init") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        CHECK_EQ(args.num_args, 2) << "The expected of arguments are: "
                                   << "runtime::Module mod and Map<int, Target> targets";
        void* mod = args[0];
        Map<Integer, tvm::Target> tmp = args[1];
        TargetsMap targets;
        for (const auto& it : tmp) {
          auto dev_type = it.first.as<tir::IntImmNode>();
          CHECK(dev_type);
          targets[dev_type->value] = it.second;
        }
        codegen_ =
            std::make_shared<GraphRuntimeCodegen>(reinterpret_cast<runtime::Module*>(mod), targets);
      });
    } else {
      return PackedFunc([](TVMArgs args, TVMRetValue* rv) {});
    }
  }

 private:
  std::shared_ptr<AotRuntimeCodegen> codegen_;
};

runtime::Module CreateGraphCodegenMod() {
  auto ptr = make_object<GraphRuntimeCodegenModule>();
  return runtime::Module(ptr);
}

TVM_REGISTER_GLOBAL("relay.build_module._GraphRuntimeCodegen")
    .set_body([](TVMArgs args, TVMRetValue* rv) { *rv = CreateGraphCodegenMod(); });

}  // namespace runtime
}  // namespace tvm
