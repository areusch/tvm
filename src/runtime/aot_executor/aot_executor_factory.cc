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
 * \file aot_executor_factory.cc
 * \brief Graph executor factory implementations
 */

#include "./aot_executor_factory.h"

#include <tvm/runtime/container/string.h>
#include <tvm/runtime/device_api.h>
#include <tvm/runtime/registry.h>

#include <iterator>
#include <vector>

namespace tvm {
namespace runtime {

AotExecutorFactory::AotExecutorFactory(
    const std::string& graph_json,
    const std::unordered_map<std::string, tvm::runtime::NDArray>& params,
    const std::string& target_str,
    const std::string& module_name) {
  graph_json_ = graph_json;
  params_ = params;
  module_name_ = module_name;
  target_str_ = target_str;
}

PackedFunc AotExecutorFactory::GetFunction(
    const std::string& name, const tvm::runtime::ObjectPtr<tvm::runtime::Object>& sptr_to_self) {
  if (name == module_name_) {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      ICHECK_GT(args.num_args, 0) << "Must supply at least one device argument";
      std::vector<Device> devices;
      for (int i = 0; i < args.num_args; ++i) {
        devices.emplace_back(args[i].operator Device());
      }
      *rv = this->ExecutorCreate(devices);
    });
  } else if (name == "debug_create") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      ICHECK_GE(args.size(), 2);
      std::string module_name = args[0].operator String();
      ICHECK(module_name == module_name_) << "Currently we only support single model for now.";
      std::vector<Device> devices;
      for (int i = 1; i < args.num_args; ++i) {
        devices.emplace_back(args[i].operator Device());
      }
      *rv = this->DebugExecutorCreate(devices);
    });
  } else if (name == "remove_params") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      std::unordered_map<std::string, tvm::runtime::NDArray> empty_params{};
      auto exec =
          make_object<AotExecutorFactory>(this->graph_json_, empty_params, this->module_name_);
      exec->Import(this->imports_[0]);
      *rv = Module(exec);
    });
  } else if (name == "cuda_graph_create") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      std::vector<Device> devices;
      for (int i = 0; i < args.num_args; ++i) {
        devices.emplace_back(args[i].operator Device());
      }
      *rv = this->CudaAotExecutorCreate(devices);
    });
  } else {
    return PackedFunc();
  }
}

void AotExecutorFactory::SaveToBinary(dmlc::Stream* stream) {
  stream->Write(graph_json_);
  std::vector<std::string> names;
  std::vector<DLTensor*> arrays;
  for (const auto& v : params_) {
    names.emplace_back(v.first);
    arrays.emplace_back(const_cast<DLTensor*>(v.second.operator->()));
  }
  uint64_t sz = arrays.size();
  ICHECK(sz == names.size());
  stream->Write(sz);
  stream->Write(names);
  for (size_t i = 0; i < sz; ++i) {
    tvm::runtime::SaveDLTensor(stream, arrays[i]);
  }
  stream->Write(module_name_);
}

Module AotExecutorFactory::ExecutorCreate(const std::vector<Device>& devs) {
  auto exec = make_object<AotExecutor>();
  exec->Init(this->graph_json_, this->imports_[0], devs, PackedFunc());
  // set params
  if (!is_link_params()) {
    SetParams(exec.get(), this->params_);
  }
  return Module(exec);
}

Module AotExecutorFactory::DebugExecutorCreate(const std::vector<Device>& devs) {
  const PackedFunc* pf = tvm::runtime::Registry::Get("tvm.aot_executor_debug.create");
  ICHECK(pf != nullptr) << "Cannot find function tvm.aot_executor_debug.create in registry. "
                           "Do you enable debug graph executor build?";
  // Debug executor create packed function will call GetAllContexs, so we unpack the devs.
  std::vector<int> unpacked_devs;
  for (const auto& dev : devs) {
    unpacked_devs.emplace_back(dev.device_type);
    unpacked_devs.emplace_back(dev.device_id);
  }
  size_t args_size = unpacked_devs.size() + 2;
  std::vector<TVMValue> values(args_size);
  std::vector<int> codes(args_size);
  runtime::TVMArgsSetter setter(values.data(), codes.data());
  setter(0, this->graph_json_);
  setter(1, this->imports_[0]);
  for (size_t i = 0; i < unpacked_devs.size(); ++i) {
    setter(i + 2, unpacked_devs[i]);
  }
  TVMRetValue rv;
  pf->CallPacked(TVMArgs(values.data(), codes.data(), args_size), &rv);
  Module mod = rv.operator Module();
  // debug graph executor is one child class of graph executor.
  if (!is_link_params()) {
    SetParams(const_cast<AotExecutor*>(mod.as<AotExecutor>()), this->params_);
  }
  return mod;
}

Module AotExecutorFactory::CudaAotExecutorCreate(const std::vector<Device>& devs) {
  const PackedFunc* pf = tvm::runtime::Registry::Get("tvm.aot_executor_cuda_graph.create");
  ICHECK(pf != nullptr) << "Cannot find function tvm.aot_executor_cuda_graph.create in registry. "
                           "Did you set(USE_AOT_EXECUTOR_CUGRAPH=ON)?";
  std::vector<int> unpacked_devs;
  for (const auto& dev : devs) {
    unpacked_devs.emplace_back(dev.device_type);
    unpacked_devs.emplace_back(dev.device_id);
  }
  size_t args_size = unpacked_devs.size() + 2;
  std::vector<TVMValue> values(args_size);
  std::vector<int> codes(args_size);
  runtime::TVMArgsSetter setter(values.data(), codes.data());
  setter(0, this->graph_json_);
  setter(1, this->imports_[0]);
  for (size_t i = 0; i < unpacked_devs.size(); ++i) {
    setter(i + 2, unpacked_devs[i]);
  }
  TVMRetValue rv;
  pf->CallPacked(TVMArgs(values.data(), codes.data(), args_size), &rv);
  Module mod = rv.operator Module();
  if (!is_link_params()) {
    SetParams(const_cast<AotExecutor*>(mod.as<AotExecutor>()), this->params_);
  }
  return mod;
}

Module AotExecutorFactoryModuleLoadBinary(void* strm) {
  dmlc::Stream* stream = static_cast<dmlc::Stream*>(strm);
  std::string graph_json;
  std::unordered_map<std::string, tvm::runtime::NDArray> params;
  std::string module_name;
  ICHECK(stream->Read(&graph_json));
  uint64_t sz;
  ICHECK(stream->Read(&sz));
  std::vector<std::string> names;
  ICHECK(stream->Read(&names));
  ICHECK(sz == names.size());
  for (size_t i = 0; i < sz; ++i) {
    tvm::runtime::NDArray temp;
    temp.Load(stream);
    params[names[i]] = temp;
  }
  ICHECK(stream->Read(&module_name));
  auto exec = make_object<AotExecutorFactory>(graph_json, params, module_name);
  return Module(exec);
}

TVM_REGISTER_GLOBAL("tvm.aot_executor_factory.create")
    .set_body([](TVMArgs args, TVMRetValue* rv) {
      ICHECK_GE(args.num_args, 3) << "The expected number of arguments for "
                                     "aot_executor_factory.create needs at least 3, "
                                     "but it has "
                                  << args.num_args;
      // The argument order is graph_json, module, module_name, param0_name, param0_tensor,
      // [param1_name, param1_tensor], ...
      ICHECK_EQ((args.size() - 4) % 2, 0);
      std::unordered_map<std::string, tvm::runtime::NDArray> params;
      for (size_t i = 4; i < static_cast<size_t>(args.size()); i += 2) {
        std::string name = args[i].operator String();
        params[name] = args[i + 1].operator tvm::runtime::NDArray();
      }
      auto exec = make_object<AotExecutorFactory>(args[0], params, args[3], args[2]);
      exec->Import(args[1]);
      *rv = Module(exec);
    });

TVM_REGISTER_GLOBAL("runtime.module.loadbinary_AotExecutorFactory")
    .set_body_typed(AotExecutorFactoryModuleLoadBinary);

Module GraphRuntimeFactoryModuleLoadBinary(void* strm) {
  LOG(WARNING) << "You are loading a module which was built with GraphRuntimeFactory. "
               << "GraphRuntime has been renamed to AotExecutor, and support for loading "
               << "GraphRuntimeFactory modules will be removed after the next TVM release. "
               << "Please rebuild the module before then to avoid breakage.";
  return AotExecutorFactoryModuleLoadBinary(strm);
}

TVM_REGISTER_GLOBAL("runtime.module.loadbinary_GraphRuntimeFactory")
    .set_body_typed(GraphRuntimeFactoryModuleLoadBinary);

}  // namespace runtime
}  // namespace tvm
