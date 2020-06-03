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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tvm/runtime/c_runtime_api.h>
#include <tvm/runtime/crt/platform.h>
#include <tvm/runtime/crt/memory.h>

#include "ndarray.h"
#include "packed_func.h"

// Handle internal errors

static char g_last_error[1024];

void TVMAPISetLastError(const char* msg) {
  assert(strlen(msg) < sizeof(g_last_error));
  snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
}

const char* TVMGetLastError(void) { return g_last_error; }

// Manipulate NDArray on target device

int TVMArrayAlloc(const tvm_index_t* shape, int ndim, int dtype_code, int dtype_bits,
                  int dtype_lanes, int device_type, int device_id, TVMArrayHandle* out) {
  DLDataType dtype;
  dtype.code = dtype_code;
  dtype.bits = dtype_bits;
  dtype.lanes = dtype_lanes;
  DLContext ctx;
  ctx.device_type = (DLDeviceType)device_type;
  ctx.device_id = device_id;
  TVMNDArray arr = TVMNDArray_Empty(ndim, shape, dtype, ctx);
  **out = arr.dl_tensor;
  return 0;
}

int TVMArrayFree(TVMArrayHandle handle) {
  TVMNDArray arr;
  arr.dl_tensor = *handle;
  return TVMNDArray_Release(&arr);
}

int TVMDeviceAllocDataSpace(DLContext ctx, size_t nbytes, size_t alignment, DLDataType type_hint,
                            void** out_data) {
  if (alignment != 1) {
    nbytes = (nbytes + alignment - 1) / alignment * alignment;
  }

  *out_data = vmalloc(nbytes);
  return 0;
}

int TVMDeviceFreeDataSpace(TVMContext ctx, void *ptr) {
  vfree(ptr);
  return 0;
}

int TVMDeviceCopyDataFromTo(const void* from, size_t from_offset, void* to,
                            size_t to_offset, size_t num_bytes, TVMContext ctx_from,
                            TVMContext ctx_to, DLDataType type_hint,
                            TVMStreamHandle stream) {
  memcpy(((uint8_t*) to) + to_offset, ((uint8_t*) from) + from_offset, num_bytes);
  return 0;
}

int TVMSynchronize(int device_type, int device_id, TVMStreamHandle stream) {
  return 0;
}

void* SystemLibraryCreate() { return 0; }

int TVMModGetFunction(TVMModuleHandle mod, const char* func_name, int query_imports,
                      TVMFunctionHandle* out) {
  TVMModule* mod_ptr = (TVMModule*) mod;
  TVMPackedFunc pf;
  mod_ptr->GetFunction(mod_ptr, func_name, &pf);
  *out = pf.fexec;
  return 0;
}

typedef struct TVMCReturnValue {
  TVMValue* ret_val;
  int* ret_type_code;
} TVMCReturnValue;

int TVMFuncCall(TVMFunctionHandle func, TVMValue* arg_values, int* type_codes, int num_args,
                TVMValue* ret_val, int* ret_type_code) {
  TVMPackedCFunc cfunc = (TVMPackedCFunc) func;
  TVMCReturnValue ret_val_struct;
  ret_val_struct.ret_val = ret_val;
  ret_val_struct.ret_type_code = ret_type_code;
  cfunc(arg_values, type_codes, num_args, &ret_val_struct, NULL);
  return 0;
}

int TVMCFuncSetReturn(TVMRetValueHandle ret, TVMValue* value, int* type_code, int num_ret) {
  TVMCReturnValue* ret_val;
  int idx;

  ret_val = (TVMCReturnValue*) ret;
  for (idx = 0; idx < num_ret; idx++) {
    ret_val->ret_val[idx] = value[idx];
    ret_val->ret_type_code[idx] = type_code[idx];
  }

  return 0;
}

int TVMFuncFree(TVMFunctionHandle func) {
  // A no-op, since we don't actually allocate anything in GetFunction
  return 0;
}

int TVMModFree(TVMModuleHandle mod ) {
  // A no-op, since we never allocate module handles.
  return 0;
}

static TVMMutableFuncRegistry global_func_registry;

int TVMFuncGetGlobal(const char* name, TVMFunctionHandle* out) {
  *out = (TVMFunctionHandle) TVMFuncRegistry_GetCFunction(&global_func_registry.reg, name);
  if (*out == NULL) {
    char msg[26 + TVM_CRT_MAX_STRLEN_FUNCTION_NAME];
    snprintf(msg, sizeof(msg), "fail to get global: name=%s", name);
    TVMAPISetLastError(msg);
    return -1;
  }

  return 0;
}

int TVMFuncRegisterGlobal(const char* name, TVMFunctionHandle f, int override) {
  return TVMMutableFuncRegistry_Set(&global_func_registry, name, f, override != 0);
}

int TVMInitializeRuntime() {
  TVMMutableFuncRegistry_Create(&global_func_registry,
                                vmalloc(TVM_CRT_GLOBAL_FUNC_REGISTRY_SIZE_BYTES),
                                TVM_CRT_GLOBAL_FUNC_REGISTRY_SIZE_BYTES);

  TVMFuncRegisterGlobal("runtime.SystemLib", &SystemLibraryCreate, 0);
  return 0;
}
