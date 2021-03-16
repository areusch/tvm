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
 * \file error_module.cc
 * \brief Defines an error module used by RPC.
 */

#include <tvm/runtime/crt/rpc_common/error_module.h>

namespace tvm {
namespace runtime {
namespace micro_rpc {

bool ErrorModule::isValid() {
  return is_valid_;
}

void ErrorModule::Clear() {
  is_valid_ = false;
}

void ErrorModule::SetError(ErrorSource source, ErrorReason reason) {
  source_ = source;
  reason_ = reason;
  is_valid_ = true;
}

ErrorSource ErrorModule::GetErrorSource() {
  return source_;
}

ErrorReason ErrorModule::GetErrorReason() {
  return reason_;
}

}  // namespace micro_rpc
}  // namespace runtime
}  // namespace tvm
