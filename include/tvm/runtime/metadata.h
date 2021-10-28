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
 * \file tvm/runtime/metadata.h
 * \brief Defines types which can be used in Metadata.
 */
#ifndef TVM_RUNTIME_METADATA_H_
#define TVM_RUNTIME_METADATA_H_

#include <string>
#include <tvm/ir/expr.h>
#include <tvm/runtime/object.h>

namespace tvm {
namespace runtime {
namespace metadata {

class MetadataBaseNode : public ::tvm::runtime::Object {
 public:
  static constexpr const char* _type_key = "metadata.MetadataBaseNode";
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataBaseNode, ::tvm::runtime::Object);
};

class MetadataBase : public ::tvm::runtime::ObjectRef {
 public:
  TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(MetadataBase, ::tvm::runtime::ObjectRef, MetadataBaseNode);
};

template <typename C, class Ref>
class ArrayAccessor;

template <typename C, class Ref>
class ArrayIterator {
 public:
  ArrayIterator(int index, ArrayAccessor<C, Ref>* parent) : index_{index}, parent_{parent} {}

  inline Ref operator*() {
    return (*parent_)[index_];
  }

  inline ArrayIterator<C, Ref>& operator++() {
    if (index_ < parent_->size()) {
      index_++;
    }

    return *this;
  }

  inline bool operator==(const ArrayIterator<C, Ref>& other) {
    return parent_ == other.parent_ & index_ == other.index_;
  }

  inline bool operator!=(const ArrayIterator<C, Ref>& other) {
    return !operator==(other);
  }

 private:
  int index_;
  ArrayAccessor<C, Ref>* parent_;
};

template <typename C, class Ref>
class ArrayAccessor {
 public:

  template <typename T=typename std::enable_if<std::is_base_of<ObjectRef, Ref>::value>::type>
  ArrayAccessor(const C* data, int num_data, ::std::shared_ptr<::std::vector<Ref>> refs) : data_{data}, num_data_{num_data}, refs_{refs} {}

  inline size_t size() { return num_data_; }

  inline Ref operator[](int index) {
    if (index >= num_data_) {
      throw std::runtime_error("Index out of range");
    }

    if (!(*refs_)[index].defined()) {
      (*refs_)[index] = Ref(data_[index]);
    }

    return (*refs_)[index];
  }

  inline ArrayIterator<C, Ref> begin() {
    return ArrayIterator<C, Ref>{0, this};
  }

  inline ArrayIterator<C, Ref> end() {
    return ArrayIterator<C, Ref>{num_data_, this};
  }

 private:
  const C* data_;
  int num_data_;
  ::std::shared_ptr<::std::vector<Ref>> refs_;
};

template <>
class ArrayAccessor<const char*, ::std::string> {
 public:
  ArrayAccessor(const char** data) : data_{data} {}

  inline ::std::string operator[](int index) {
    return ::std::string{data_[index]};
  }

 private:
  const char** data_;
};

enum MetadataTypeIndex : uint8_t {
  kUint64 = 0,
  kInt64 = 1,
  kBool = 2,
  kString = 3,
  kHandle = 4,

};

class MetadataArrayNode : public MetadataBaseNode {
 public:
//  MetadataArray(Array<ObjectRef> array, MetadataTypeIndex type_index) : array{array}, type_index{type_index} {}
  MetadataArrayNode(ArrayNode* array, const char* c_type) : array{array}, c_type{c_type} {}

  ArrayNode* array;
  const char* c_type;
  TVM_DECLARE_BASE_OBJECT_INFO(MetadataArrayNode, MetadataBaseNode);
};

class MetadataArray : public MetadataBase {
 public:
//  MetadataArray(Array<ObjectRef> array, MetadataTypeIndex type_index);
  MetadataArray(ArrayNode* array, const char* c_type);
  TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(MetadataArray, MetadataBase, MetadataArrayNode);
};

}  // namespace metadata
}  // namespace runtime
}  // namespace tvm

#include <tvm/generated/runtime/metadata.h>

#endif  // TVM_RUNTIME_METADATA_H_
