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
 * \file tvm/runtime/c_packed_fun.h
 * \brief Defines type signatures for PackedFunc and auxiliary functions needed.
 */

#ifndef TVM_RUNTIME_C_PACKED_FUNC_H_
#define TVM_RUNTIME_C_PACKED_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <dlpack/dlpack.h>
#include <tvm/runtime/function_attributes.h>

/*! \brief Extension device types in TVM */
typedef enum {
  kDLAOCL = 5,
  kDLSDAccel = 6,
  kOpenGL = 11,
  kDLMicroDev = 13,
  kDLHexagon = 14,
  kDLWebGPU = 15
  // AddExtraTVMType which is not in DLPack here
} TVMDeviceExtType;

/*!
 * \brief The type code in used and only used in TVM FFI for argument passing.
 *
 * DLPack consistency:
 * 1) kTVMArgInt is compatible with kDLInt
 * 2) kTVMArgFloat is compatible with kDLFloat
 * 3) kDLUInt is not in ArgTypeCode, but has a spared slot
 *
 * Downstream consistency:
 * The kDLInt, kDLUInt, kDLFloat are kept consistent with the original ArgType code
 *
 * It is only used in argument passing, and should not be confused with
 * DataType::TypeCode, which is DLPack-compatible.
 *
 * \sa tvm::runtime::DataType::TypeCode
 */
typedef enum {
  kTVMArgInt = kDLInt,
  kTVMArgFloat = kDLFloat,
  kTVMOpaqueHandle = 3U,
  kTVMNullptr = 4U,
  kTVMDataType = 5U,
  kDLDevice = 6U,
  kTVMDLTensorHandle = 7U,
  kTVMObjectHandle = 8U,
  kTVMModuleHandle = 9U,
  kTVMPackedFuncHandle = 10U,
  kTVMStr = 11U,
  kTVMBytes = 12U,
  kTVMNDArrayHandle = 13U,
  kTVMObjectRValueRefArg = 14U,
  // Extension codes for other frameworks to integrate TVM PackedFunc.
  // To make sure each framework's id do not conflict, use first and
  // last sections to mark ranges.
  // Open an issue at the repo if you need a section of code.
  kTVMExtBegin = 15U,
  kTVMNNVMFirst = 16U,
  kTVMNNVMLast = 20U,
  // The following section of code is used for non-reserved types.
  kTVMExtReserveEnd = 64U,
  kTVMExtEnd = 128U,
} TVMArgTypeCode;

/*!
 * \brief Byte array type used to pass in byte array
 *  When kTVMBytes is used as data type.
 */
typedef struct {
  const char* data;
  size_t size;
} TVMByteArray;

/*!
 * \brief Union type of values
 *  being passed through API and function calls.
 */
typedef union {
  int64_t v_int64;
  double v_float64;
  void* v_handle;
  const char* v_str;
  DLDataType v_type;
  DLDevice v_device;
} TVMValue;


/*!
 * \brief Signature for backend functions exported as DLL.
 *
 * \param args The arguments
 * \param type_codes The type codes of the arguments
 * \param num_args Number of arguments.
 * \param out_ret_value The output value of the the return value.
 * \param out_ret_tcode The output type code of the return value.
 * \param resource_handle Pointer to associated resource.
 *
 * \return 0 if success, -1 if failure happens, set error via TVMPackedFuncSetLastError.
 */
typedef int (*TVMBackendPackedCFunc)(TVMValue* args, int* type_codes, int num_args,
                                     TVMValue* out_ret_value, int* out_ret_tcode,
                                     void* resource_handle);

/*!
 * \brief Used for implementing C API function.
 *  Set last error message before return.
 * \param msg The error message to be set.
 */
TVM_DLL void TVMPackedFuncSetLastError(const char* msg);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TVM_RUNTIME_C_PACKED_FUNC_H_
