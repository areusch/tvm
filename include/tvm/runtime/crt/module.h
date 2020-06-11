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
 * \file src/runtime/crt/module.h
 * \brief Runtime container of the functions
 */
#ifndef TVM_RUNTIME_CRT_MODULE_H_
#define TVM_RUNTIME_CRT_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tvm/runtime/c_backend_api.h>

typedef uint16_t tvm_module_index_t;

typedef uint16_t tvm_function_index_t;

typedef struct TVMFuncRegistry {
  /*! \brief Names of registered functions, concatenated together and separated by \0.
   * An additional \0 is present at the end of the concatenated blob to mark the end.
   * Byte 0 is the number of functions in `funcs`.
   */
  const char* names;

  /*! \brief Function pointers, in the same order as their names in `names`. */
  TVMBackendPackedCFunc* funcs;
} TVMFuncRegistry;

/*!
 * \brief Get packed function from registry by name.
 *
 * \param reg TVMFunctionRegistry instance that contains the function.
 * \param name The function name
 * \param function_index Pointer to receive the 0-based index of the function in the registry, if it
 *     was found. Unmodified otherwise.
 * \return -1 if the function was not found.
 */
int TVMFuncRegistry_Lookup(const TVMFuncRegistry* reg, const char* name, tvm_function_index_t* function_index);

/*!
 * \brief Fetch TVMBackendPackedCFunc given a function index
 *
 * \param reg TVMFunctionRegistry instance that contains the function.
 * \param index Index of the function.
 * \param out_func Pointer which receives the function pointer at `index`, if a valid
 *      index was given. Unmodified otherwise.
 * \return 0 on success. -1 if the index was invalid.
 */
int TVMFuncRegistry_GetByIndex(const TVMFuncRegistry* reg, tvm_function_index_t index, TVMBackendPackedCFunc* out_func);
/*!
 * \brief Module container of TVM.
 */
typedef struct TVMModule {
  /*! \brief The function registry associated with this mdoule. */
  const TVMFuncRegistry* registry;
} TVMModule;

/*! \brief Entry point for the system lib module. */
const TVMModule* TVMSystemLibEntryPoint(void);

#ifdef __cplusplus
}
#endif

#endif  // TVM_RUNTIME_CRT_MODULE_H_
