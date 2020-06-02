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
 * \file rpc_procotol.h
 * \brief Common header defining the communication code used in the RPC protocol.
 */
#ifndef TVM_RUNTIME_RPC_RPC_PROTOCOL_C_H_
#define TVM_RUNTIME_RPC_RPC_PROTOCOL_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

const char* tvm_rpc__protocol_ver = "0.7.0";

typedef uint8_t RPCCode;

// RPC Codes  -->
#define tvm_rpc__rpc_code_None             0x00
#define tvm_rpc__rpc_code_Shutdown         0x01
#define tvm_rpc__rpc_code_InitServer       0x02
#define tvm_rpc__rpc_code_CallFunc         0x03
#define tvm_rpc__rpc_code_Return           0x04
#define tvm_rpc__rpc_code_Exception        0x05
#define tvm_rpc__rpc_code_CopyFromRemote   0x06
#define tvm_rpc__rpc_code_CopyToRemote     0x07
#define tvm_rpc__rpc_code_CopyAck          0x08
// The following are syscall code that can send over CallRemote
#define tvm_rpc__rpc_code_SyscallCodeStart 0x09
#define tvm_rpc__rpc_code_GetGlobalFunc    (tvm_rpc__rpc_code_SyscallCodeStart + 0x00)
#define tvm_rpc__rpc_code_FreeHandle       (tvm_rpc__rpc_code_SyscallCodeStart + 0x01)
#define tvm_rpc__rpc_code_DevSetDevice     (tvm_rpc__rpc_code_SyscallCodeStart + 0x02)
#define tvm_rpc__rpc_code_DevGetAttr       (tvm_rpc__rpc_code_SyscallCodeStart + 0x03)
#define tvm_rpc__rpc_code_DevAllocData     (tvm_rpc__rpc_code_SyscallCodeStart + 0x04)
#define tvm_rpc__rpc_code_DevFreeData      (tvm_rpc__rpc_code_SyscallCodeStart + 0x05)
#define tvm_rpc__rpc_code_DevStreamSync    (tvm_rpc__rpc_code_SyscallCodeStart + 0x06)
#define tvm_rpc__rpc_code_CopyAmongRemote  (tvm_rpc__rpc_code_SyscallCodeStart + 0x07)
// <-- RPC Codes

typedef uint8_t RPCStatus;

// RPC Server Status Codes -->
#define tvm_rpc__rpc_status_Success                         0x00
#define tvm_rpc__rpc_status_InvalidTypeCodeObject           0x01
#define tvm_rpc__rpc_status_InvalidTypeCodeNDArray          0x02
#define tvm_rpc__rpc_status_InvalidDLTensorFieldStride      0x03
#define tvm_rpc__rpc_status_InvalidDLTensorFieldByteOffset  0x04
#define tvm_rpc__rpc_status_UnknownTypeCode                 0x05
#define tvm_rpc__rpc_status_UnknownRPCCode                  0x06
#define tvm_rpc__rpc_status_RPCCodeNotSupported             0x07
#define tvm_rpc__rpc_status_UnknownRPCSyscall               0x08
#define tvm_rpc__rpc_status_CheckError                      0x09
#define tvm_rpc__rpc_status_ReadError                       0x0a
#define tvm_rpc__rpc_status_WriteError                      0x0b
#define tvm_rpc__rpc_status_AllocError                      0x0c
// <-- RPC Server Status Codes

const char* tvm_rpc__rpc_status_to_string(RPCStatus status);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TVM_RUNTIME_RPC_RPC_PROTOCOL_C_H_
