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

/*
 * \file tvm/runtime/function_attributes.h
 * \brief Defines common function attributes used in the TVM public APIs.
 */

#ifndef TVM_RUNTIME_FUNCTION_ATTRIBUTES_H_
#define TVM_RUNTIME_FUNCTION_ATTRIBUTES_H_

// Macros to do weak linking
#ifdef _MSC_VER
#define TVM_WEAK __declspec(selectany)
#else
#define TVM_WEAK __attribute__((weak))
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define TVM_DLL EMSCRIPTEN_KEEPALIVE
#endif

#ifndef TVM_DLL
#ifdef _WIN32
#ifdef TVM_EXPORTS
#define TVM_DLL __declspec(dllexport)
#else
#define TVM_DLL __declspec(dllimport)
#endif
#else
#define TVM_DLL __attribute__((visibility("default")))
#endif
#endif


#endif  // TVM_RUNTIME_FUNCTION_ATTRIBUTES_H_
