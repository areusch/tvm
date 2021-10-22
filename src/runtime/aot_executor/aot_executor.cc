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
 * \brief Defines an implementation of Module-based Model Runtime Interface that works with
 *        Ahead-of-Time compilation.
 * \file aot_executor.cc
 */

#include "aot_executor.h"

namespace tvm {
namespace runtime {

AotExecutor::AotExecutor(const runtime::Metadata& meta_data, tvm::runtime::Module module,
                         const std::vector<Device>& devs) :
    meta_data_{meta_data}, module_{module}, devices_{devs} {

  for (int i = 0; i < meta_data_.input_names.size(); ++i) {
    // TODO(areusch): Encode device information in Metadata.
    args_.emplace_back(NDArray::Empty(ShapeTuple(meta_data_.input_shapes[i]), meta_data_.input_dtype[i], devices_[0]));
  }
}

PackedFunc AotExecutor::GetFunction(std::string name, const ObjectPtr<Object>& sptr_to_self) {
  // Return member functions during query.
  if (name == "set_input") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      if (String::CanConvertFrom(args[0])) {
        int in_idx = this->GetInputIndex(args[0].operator String());
        if (in_idx >= 0) this->SetInput(in_idx, args[1]);
      } else {
        this->SetInput(args[0], args[1]);
      }
    });
  } else if (name == "set_input_zero_copy") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      if (String::CanConvertFrom(args[0])) {
        int in_idx = this->GetInputIndex(args[0].operator String());
        if (in_idx >= 0) this->SetInputZeroCopy(in_idx, args[1]);
      } else {
        this->SetInputZeroCopy(args[0], args[1]);
      }
    });
  } else if (name == "set_output_zero_copy") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      if (String::CanConvertFrom(args[0])) {
        int out_idx = this->GetOutputIndex(args[0].operator String());
        if (out_idx >= 0) this->SetOutputZeroCopy(out_idx, args[1]);
      } else {
        this->SetOutputZeroCopy(args[0], args[1]);
      }
    });
  } else if (name == "get_output") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      if (args.num_args == 2) {
        this->CopyOutputTo(args[0], args[1]);
      } else {
        *rv = this->GetOutput(args[0]);
      }
    });
  } else if (name == "get_input") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      int in_idx = 0;
      if (String::CanConvertFrom(args[0])) {
        in_idx = this->GetInputIndex(args[0].operator String());
      } else {
        in_idx = args[0];
      }
      if (in_idx >= 0) {
        *rv = this->GetInput(in_idx);
      }
    });
  } else if (name == "get_num_outputs") {
    return PackedFunc(
        [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = this->NumOutputs(); });
  } else if (name == "get_num_inputs") {
    return PackedFunc(
        [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = this->NumInputs(); });
  } else if (name == "run") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { this->Run(); });
  } else if (name == "run_from_inputs") {
    return PackedFunc(
        [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
          CHECK(args.size() % 2 == 0)
              << "Number of arguments to run_from_inputs must be an even number of key-value pairs";
          Device host{static_cast<DLDeviceType>(args[0].operator int()), args[1].operator int()};
          for (int i = 2; i < args.size(); i += 2) {
            if (String::CanConvertFrom(args[i])) {
              int in_idx = this->GetInputIndex(args[i].operator String());
              if (in_idx >= 0) {
                this->SetInput(in_idx, args[i + 1]);
              } else {
                LOG(FATAL) << args[i].operator String() << " is not a valid input name";
              }
            } else {
              this->SetInput(args[i], args[i + 1]);
            }
          }
          this->Run();
          Array<NDArray> outputs;
          for (int i = 0; i < this->NumOutputs(); i++) {
            NDArray out = this->GetOutput(i);
            NDArray a = NDArray::Empty(out.Shape(), out.DataType(), host);
            a.CopyFrom(out);
            outputs.push_back(a);
          }
          *rv = outputs;
        });
  } else if (name == "get_input_index") {
    return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
      CHECK(String::CanConvertFrom(args[0])) << "Input key is not a string";
      *rv = this->GetInputIndex(args[0].operator String());
    });
  } else {
    return PackedFunc();
  }
}

void AotExecutor::Run() {
  auto pf = module_.GetFunction(meta_data_.mod_name);
  ICHECK_NE(pf, nullptr) << "Module entrypoint is not defined";

  const int num_args = meta_data_.input_names.size() + 1; // return values
  TVMValue call_values[num_args];
  TVMValue call_type_codes[num_args];
  for (int i = 0; i < meta_data_.input_names.size(); ++i) {
    auto managed = args_[i].ToDLPack();
    call_values[i] = managed->dl_tensor;
    call_type_codes[i] = kTVMDLTensorHandle;
  }

  TVMArgs args{call_values, call_type_codes, num_args};
  TVMRetValue rv;
  pf(args, &rv);
}

int AotExecutor::GetInputIndex(const std::string& name) {
  return std::find(meta_data_.input_names.begin(), meta_data_.input_names.end(), name);
}

int AotExecutor::GetOutputIndex(const std::string& name) {
  return 0;
}

void AotExecutor::SetInput(int index, DLTensor* data_ref) {
  args_[index].CopyFrom(data_ref);
}

void AotExecutor::SetInputZeroCopy(int index, DLTensor* data_ref) {
  ICHECK(false) << "not implemented";
}

void AotExecutor::SetOutputZeroCopy(int index, DLTensor* data_ref) {
  ICHECK(false) << "not implemented";
}

int AotExecutor::NumOutputs() {
  return 1;
}

int AotExecutor::NumInputs() {
  return args_.size() - 1;
}

NDArray AotExecutor::GetInput(int index) const {
  return args_[index];
}

NDArray AotExecutor::GetOutput(int index) const {
  return args_[args_.size() - 1];
}

void AotExecutor::CopyOutputTo(int index, DLTensor* data_out) {
  GetOutput(index).CopyTo(data_out);
}

}  // namespace runtime
}  // namespace tvm
