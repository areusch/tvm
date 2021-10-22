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
 * \file const_loader_module.h
 * \brief Defines an interface to use the ConstLoaderModule.
 */

#ifndef TVM_RUNTIME_CONST_LOADER_MODULE_H_
#define TVM_RUNTIME_CONST_LOADER_MODULE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <tvm/runtime/ndarray.h>

namespace tvm {
namespace runtime {

/*!
 * \brief Create a constLoader module object.
 *
 * \param metadata The variable name to ndarray mapping.
 * \param sym_vars The symbol to the list of required constant variables
 * mapping.
 *
 * \return The created ConstLoaderModule.
 */
Module ConstLoaderModuleCreate(
    const std::unordered_map<std::string, NDArray>& metadata,
    const std::unordered_map<std::string, std::vector<std::string>>& sym_vars);

}  // namespace runtime
}  // namespace tvm

#endif  // TVM_RUNTIME_CONST_LOADER_MODULE_H_
