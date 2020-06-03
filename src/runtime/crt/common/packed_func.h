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
 * \file tvm/runtime/packed_func.h
 * \brief Type-erased function used across TVM API.
 */
#ifndef TVM_RUNTIME_CRT_PACKED_FUNC_H_
#define TVM_RUNTIME_CRT_PACKED_FUNC_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <tvm/runtime/c_runtime_api.h>

#include "crt_config.h"
#include "module.h"

DLDataType String2DLDataType(const char* s);

typedef struct TVMArgs {
  TVMValue values[TVM_CRT_MAX_ARGS];
  int tcodes[TVM_CRT_MAX_ARGS]; /* Data type should be identical to type_codes in TVMPackedCFunc */
  uint32_t values_count;
} TVMArgs;

TVMArgs TVMArgs_Create(TVMValue* values, uint32_t* tcodes, uint32_t values_count);

typedef struct TVMPackedFunc {
  char name[TVM_CRT_MAX_FUNCTION_NAME_LENGTH_BYTES];
  TVMPackedCFunc fexec;
  TVMArgs args;
  void (*Call)(struct TVMPackedFunc* pf);
  void (*SetArgs)(struct TVMPackedFunc* pf, const struct TVMArgs* args);
} TVMPackedFunc;

typedef struct TVMFuncRegistry {
  /*! \brief Names of registered functions, concatenated together and separated by \0.
   * An additional \0 is present at the end of the concatenated blob to mark the end.
   */
  const char* names;

  /*! \brief Function pointers, in the same order as their names in `names`. */
  TVMPackedCFunc* funcs;
} TVMFuncRegistry;

/*!
 * \brief Get packed function from registry by name.
 *
 * \param reg TVMFunctionRegistry instance that contains the function.
 * \param name The function name
 * \return A non-NULL function pointer if a match was found. NULL otherwise.
 */
TVMPackedCFunc TVMFuncRegistry_GetCFunction(TVMFuncRegistry* reg, const char* name);

/*!
 * \brief Populate TVMPackedFunc from registry by name.
 *
 * \param name The name of the function.
 * \param pf The result function. This function will populate a no-op PackedFUnc if the function
 *     name does not exist.
 * \return 0 if the function was found, -1 otherwise.
 */
int TVMFuncRegistry_GetPackedFunc(TVMFuncRegistry* reg, const char* name, TVMPackedFunc* func);

typedef struct TVMMutableFuncRegistry {
  TVMFuncRegistry reg;

  /*! \brief maximum number of functions in this registry. */
  size_t max_functions;
} TVMMutableFuncRegistry;

/*!
 * \brief Create a new mutable function registry from a block of memory.
 *
 * \param reg TVMMutableFuncRegistry to create.
 * \param buffer Backing memory available for this function registry.
 * \param buffer_size_bytes Number of bytes available in buffer.
 */
void TVMMutableFuncRegistry_Create(TVMMutableFuncRegistry* reg, uint8_t* buffer, size_t buffer_size_bytes);

/*!
 * \brief Add or set a function in the registry.
 *
 * \param reg The mutable function registry to affect.
 * \param name Name of the function.
 * \param func The function pointer.
 * \param override non-zero if an existing entry should be overridden.
 * \return 0 if the function was set successfully, -1 if out of space.
 */
int TVMMutableFuncRegistry_Set(TVMMutableFuncRegistry* reg, const char* name, TVMPackedCFunc func, int override);

#endif  // TVM_RUNTIME_CRT_PACKED_FUNC_H_
