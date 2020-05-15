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
 * \file micro_library.h
 * \brief Defines the MicroLibrary class.
 */
#ifndef TVM_RUNTIME_MICRO_MICRO_LIBRARY_H_
#define TVM_RUNTIME_MICRO_MICRO_LIBRARY_H_

#include <tvm/runtime/object.h>

namespace tvm {
namespace runtime {

class MicroLibrary : public ObjectRef {
 public:
  MicroLibrary() {}
  explicit MicroLibrary(ObjectRef<Object> n) : ObjectRef(n) {}

  /*!
   * \brief Get packed function from current module by name.
   *
   * \param name The name of the function.
   * \param query_imports Whether also query dependency modules.
   * \return The result function.
   *  This function will return PackedFunc(nullptr) if function do not exist.
   * \note Implemented in packed_func.cc
   */
  inline PackedFunc GetFunction(const std::string& name, bool query_imports = false);


  using ContainerType = MicroLibraryNode;
  friend class MicroLibraryNode;
};



class MicroLibraryNode : public Object {

};

}  // namespace runtime
}  // namespace tvm

#endif
