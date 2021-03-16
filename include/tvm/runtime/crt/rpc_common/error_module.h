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
 * \file tvm/runtime/crt/rpc_common/error_module.h
 * \brief Defines an error module used by RPC.
 */

#ifndef TVM_RUNTIME_CRT_RPC_COMMON_ERROR_MODULE_H_
#define TVM_RUNTIME_CRT_RPC_COMMON_ERROR_MODULE_H_

#include <inttypes.h>
#include <sys/types.h>

namespace tvm {
namespace runtime {
namespace micro_rpc {

const uint8_t kErrorModuleMagicNumber = 0xAA;

enum class ErrorSource : uint8_t {
  kTVMPlatform = 0x00,
  kZephyr = 0x01,
};

enum class ErrorReason : uint8_t {
  kReset = 0x00,
};

class ErrorModule {
  public:
    ErrorModule(ErrorSource source, ErrorReason reason)
      : source_{source}, reason_{reason}, is_valid_{true} {}

    ErrorModule()
      : source_{(ErrorSource)0x00}, reason_{(ErrorReason)0x00}, 
      is_valid_{false} {}

    void* operator new(size_t count, void* ptr) { return ptr; }

    bool isValid();
    void SetError(ErrorSource source, ErrorReason reason);
    ErrorSource GetErrorSource();
    ErrorReason GetErrorReason();
    void Clear();

  private:
    /*! \brief reset source. */
    ErrorSource source_;

    /*! \brief reset reason. */
    ErrorReason reason_;

    /*! \brief crc. */
    bool is_valid_;
};

}  // namespace micro_rpc
}  // namespace runtime
}  // namespace tvm

#endif  // TVM_RUNTIME_CRT_RPC_COMMON_ERROR_MODULE_H_
