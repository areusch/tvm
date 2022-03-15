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
 * \file tvm/target/metadata_utils.h
 * \brief Declares utilty functions and classes for emitting metadata.
 */
#ifndef TVM_TARGET_METADATA_UTILS_H_
#define TVM_TARGET_METADATA_UTILS_H_

#include <string>
#include <tuple>
#include <vector>
#include <tvm/runtime/data_type.h>
#include <tvm/runtime/ndarray.h>
#include <tvm/runtime/object.h>
#include "metadata.h"

namespace tvm {
namespace codegen {

namespace metadata {

std::string address_from_parts(const std::vector<std::string>& parts);
static constexpr const char* kMetadataGlobalSymbol = "kTvmgenMetadata";

/*!
 * \brief Post-order traverse metadata to discover arrays which need to be forward-defined.
 */
class DiscoverArraysVisitor : public AttrVisitor {
 public:
  /*! \brief Models a single array discovered in this visitor.
   * Conatains two fields:
   *  0. An address which uniquely identifies the array in this Metadata instance.
   *  1. The discovered MetadataArray.
   */
  using DiscoveredArray = std::tuple<std::string, runtime::metadata::MetadataArray>;
  explicit DiscoverArraysVisitor(std::vector<DiscoveredArray>* queue);

  void Visit(const char* key, double* value) final;
  void Visit(const char* key, int64_t* value) final;
  void Visit(const char* key, uint64_t* value) final;
  void Visit(const char* key, int* value) final;
  void Visit(const char* key, bool* value) final;
  void Visit(const char* key, std::string* value) final;
  void Visit(const char* key, DataType* value) final;
  void Visit(const char* key, runtime::NDArray* value) final;
  void Visit(const char* key, void** value) final;

  void Visit(const char* key, ObjectRef* value) final;
 private:
  /*! \brief The queue to be filled with discovered arrays. */
  std::vector<DiscoveredArray>* queue_;

  /*! \brief Tracks the preceding address pieces. */
  std::vector<std::string> address_parts_;
};

/*!
 * \brief Post-order traverse Metadata to discover all complex types which need to be forward-defined.
 * This visitor finds one defined() MetadataBase instance for each unique subclass present inside
 * Metadata in the order in which the subclass was first discovered.
 */
class DiscoverComplexTypesVisitor : public AttrVisitor {
 public:
  /*! \brief Models a single complex type discovered in this visitor.
   * Contains two fields:
   *  0. The struct_name for this Metadata instance.
   *  1. The discovered MetadataArray.
   */
  using DiscoveredComplexType = std::tuple<std::string, runtime::metadata::MetadataBase>;

  /*! \brief Construct a new instance.
   * \param queue An ordered map which holds the
   */
  explicit DiscoverComplexTypesVisitor(std::vector<runtime::metadata::MetadataBase>* queue) : queue_{queue} {}

  void Visit(const char* key, double* value) final;
  void Visit(const char* key, int64_t* value) final;
  void Visit(const char* key, uint64_t* value) final;
  void Visit(const char* key, int* value) final;
  void Visit(const char* key, bool* value) final;
  void Visit(const char* key, std::string* value) final;
  void Visit(const char* key, DataType* value) final;
  void Visit(const char* key, runtime::NDArray* value) final;
  void Visit(const char* key, void** value) final;

  void Visit(const char* key, ObjectRef* value) final;

  void Discover(runtime::metadata::MetadataBase metadata);
 private:
  bool DiscoverType(std::string type_key);

  void DiscoverInstance(runtime::metadata::MetadataBase md);

  std::vector<runtime::metadata::MetadataBase>* queue_;

  /*! \brief map type_index to index in queue_. */
  std::unordered_map<std::string, int> type_key_to_position_;
};

}  // namespace metadata
}  // namespace codegen
}  // namespace tvm

#endif  // TVM_TARGET_METADATA_UTILS_H_
