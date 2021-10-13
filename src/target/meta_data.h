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

#include <tvm/runtime/object.h>

namespace tvm {
namespace codegen {
namespace metadata {

class MetadataBaseNode : public Object {
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.MetadataBaseNode";
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataBase, Object);
};

template <int N, int M>
static constexpr const char[N + M - 1] constexpr_strcat(const char[N] a, const char[M] b) {
                             return {a, b};
                           }

template <typename T, std::enable_if<std::is_base_of<ObjectRef, T> > >
class MetadataArrayNode : public Object {
 public:
  Array<T> data;

  const char* get_element_type_key() {
    return T::_type_key;
  }

  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = constexpr_strcat("codegen.metadata.MetadataArray.", T::_type_key);
};

template <typename T, std::enable_if<std::is_base_of<ObjectRef, T> > >
class MetadataArray : public ObjectRef {
public:
  TVM_DEFINE_OBJECT_REF_METHODS(MetadataArray<T>, ObjectRef, ArrayNode);
};

class MetadataBase : public ObjectRef {
 public:
  inline MetadataBaseNode* get_mutable() {
    return ObjectRef::get_mutable();
  }

  TVM_DEFINE_OBJECT_REF_METHODS(MetadataBase, ObjectRef, MetadataBaseNode);
};

class ParameterInfoNode : public MetadataBaseNode {
 public:
  String relay_name_hint;
  String tir_name_hint;
  MetadataArray<Integer> shape;
  Integer ndim;
  DLDataType dtype;

  void VisitAttrs(AttrVisitor* v) override {
    v->Visit("relay_name_hint", &relay_name_hint);
    v->Visit("tir_name_hint", &tir_name_hint);
    v->Visit("shape_name_hint", &shape);
    v->Visit("ndim", &ndim);
    v->Visit("dtype", &dtype);
  }

  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.ParameterInfoNode";
  TVM_DECLARE_BASE_OBJECT_INFO(ParameterInfoNode, MetadataBaseNode);
};

class ParameterInfo : public ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(ParameterInfo, ObjectRef, ParameterInfoNode);
};

class FunctionInfoNode : public Object {
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

  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.FunctionInfoNode";
  TVM_DECLARE_BASE_OBJECT_INFO(ParameterInfoNode, MetadataBaseNode);
};

class FunctionInfo : public ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(FunctionInfo, ObjectRef, FunctionInfoNode);
};

class MetadataNode : public Object {
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

  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "codegen.metadata.MetadataNode";
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataNode, MetadataBaseNode);
};

class Metadata : public ObjectRef {
  TVM_DEFINE_OBJECT_REF_METHODS(Metadata, ObjectRef, MetadataNode);
};

}  // namespace metadata
}  // namespace codegen
}  // namespace tvm

#endif  // TVM_TARGET_META_DATA_H_
