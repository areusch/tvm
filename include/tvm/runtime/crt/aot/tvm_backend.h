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
 * \file include/tvm/runtime/crt/aot/tvm_backend.h
 * \brief Backend functions for the AOT executor
 *
 * These are not designed to user-facing and may change without warning
 */

#ifndef TVM_RUNTIME_CRT_AOT_TVM_BACKEND_H_
#define TVM_RUNTIME_CRT_AOT_TVM_BACKEND_H_

#include <stddef.h>
#include <stdint.h>

#include <tvm/runtime/c_backend_api.h>
#include <tvm/runtime/crt/error_codes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! Memory alignment for allocator */
#ifndef TVM_RUNTIME_ALLOC_ALIGNMENT
#define TVM_RUNTIME_ALLOC_ALIGNMENT 16
#endif

#define TVM_DLL

/*!
 * \brief Packed function signature definition
 */
typedef int32_t(tvm_function_t)(void* args, void* arg_type_ids, int32_t num_args,
                                void* out_ret_value, void* out_ret_tcode, void* resource_handle);

/*!
 * \brief Workspace memory structure
 */
typedef struct {
  uint8_t* next_alloc;   /** Pointer to the next block of bytes to allocate */
  uint8_t* workspace;    /** Pointer to start of the workspace */
  size_t workspace_size; /** Total number of bytes in the workspace */
} tvm_workspace_t;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TVM_RUNTIME_CRT_AOT_TVM_BACKEND_H_
