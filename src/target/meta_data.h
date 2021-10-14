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
 * \file meta_data.h
 * \brief Defines all metadata which can be exported from compiler to runtime.
 */

#ifndef TVM_TARGET_META_DATA_H_
#define TVM_TARGET_META_DATA_H_

#include <tvm/ir/expr.h>
#include <tvm/runtime/container/array.h>
#include <tvm/runtime/container/string.h>
#include <tvm/runtime/object.h>

namespace tvm {
namespace codegen {
namespace metadata {

class MetadataBaseNode : public ::tvm::runtime::Object {
 public:
  virtual void VisitAttrs(AttrVisitor* v) = 0;

 private:
  static constexpr const uint32_t _type_index = ::tvm::runtime::TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.MetadataBaseNode";
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataBaseNode, Object);
};

template <int N>
struct c_string {
  const char data[N];
};

template <int N, int M>
static constexpr c_string<N + M - 1> constexpr_strcat(const char a[N], const char b[M]) {
  c_string<N + M - 1> c_str;
  for (int i = 0; i < N - 1; ++i) {
    c_str.data[i] = a[i];
  }
  static_assert(a[N - 1] == '\0');
  for (int j = 0; j < M; ++j) {
    c_str.data[N - 1 + j] = b[j];
  }
  return c_str;
}

template <class T, std::enable_if_t<std::is_base_of<::tvm::runtime::ObjectRef, T>::value, bool> = true>
class MetadataArrayNode : public ::tvm::runtime::Object {
 public:
  ::tvm::runtime::Array<T> data;

  const char* get_element_type_key() {
    return T::_type_key;
  }

  static constexpr const uint32_t _type_index = ::tvm::runtime::TypeIndex::kDynamic;
  static constexpr const char* _type_key = constexpr_strcat("codegen.metadata.MetadataArray.", T::_type_key).data;
};

template <typename T, std::enable_if_t<std::is_base_of<::tvm::runtime::ObjectRef, T>::value, bool> = true>
class MetadataArray : public ::tvm::runtime::ObjectRef {
public:
  TVM_DEFINE_OBJECT_REF_METHODS(MetadataArray, ObjectRef, ArrayNode);
};

class MetadataBase : public ::tvm::runtime::ObjectRef {
 public:
  inline MetadataBaseNode* get_mutable() {
    return static_cast<MetadataBaseNode*>(ObjectRef::get_mutable());
  }

  TVM_DEFINE_OBJECT_REF_METHODS(MetadataBase, ObjectRef, MetadataBaseNode);
};

class ParameterInfoNode : public MetadataBaseNode {
 public:
  String relay_name_hint;
  String tir_name_hint;
  MetadataArray<Integer> shape;
  Integer ndim;
  DataType dtype;

  void VisitAttrs(AttrVisitor* v) override {
    v->Visit("relay_name_hint", &relay_name_hint);
    v->Visit("tir_name_hint", &tir_name_hint);
    v->Visit("shape_name_hint", &shape);
    v->Visit("ndim", &ndim);
    v->Visit("dtype", &dtype);
  }

  static constexpr const uint32_t _type_index = ::tvm::runtime::TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.ParameterInfoNode";
  TVM_DECLARE_BASE_OBJECT_INFO(ParameterInfoNode, MetadataBaseNode);
};

class ParameterInfo : public ::tvm::runtime::ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(ParameterInfo, ObjectRef, ParameterInfoNode);
};

class FunctionInfoNode : public ::tvm::runtime::Object {
 public:
  String function_name;
  MetadataArray<ParameterInfo> params;
  Integer num_params;
  Integer num_inputs;

  void VisitAttrs(AttrVisitor* v) override {
    v->Visit("function_name", &function_name);
    v->Visit("params", &params);
    v->Visit("num_params", &num_params);
    v->Visit("num_inputs", &num_inputs);
  }

  static constexpr const uint32_t _type_index = ::tvm::runtime::TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.FunctionInfoNode";
  TVM_DECLARE_BASE_OBJECT_INFO(ParameterInfoNode, MetadataBaseNode);
};

class FunctionInfo : public ::tvm::runtime::ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(FunctionInfo, ObjectRef, FunctionInfoNode);
};

class MetadataNode : public ::tvm::runtime::Object {
 public:
  Integer version;
  MetadataArray<FunctionInfo> functions;
  String module_name;
  String target;

  void VisitAttrs(AttrVisitor* v) override {
    v->Visit("version", &version);
    v->Visit("functions", &functions);
    v->Visit("module_name", &module_name);
    v->Visit("target", &target);
  }

  static constexpr const uint32_t _type_index = ::tvm::runtime::TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.MetadataNode";
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataNode, MetadataBaseNode);
};

class Metadata : public ::tvm::runtime::ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(Metadata, ObjectRef, MetadataNode);
};

}  // namespace metadata
}  // namespace codegen
}  // namespace tvm

#endif  // TVM_TARGET_META_DATA_H_
