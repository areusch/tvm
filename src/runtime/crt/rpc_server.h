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
 * \file pc_server.h
 * \brief C runtime RPC server implementation.
 *
 * \note This file do not depend on c++ std or c std,
 *       and only depends on TVM's C runtime API.
 */
#ifndef TVM_RUNTIME_RPC_MINRPC_MINRPC_SERVER_H_
#define TVM_RUNTIME_RPC_MINRPC_MINRPC_SERVER_H_

#include <dmlc/endian.h>
#include <tvm/runtime/c_runtime_api.h>

#include <inttypes.h>
#include <stdlib.h>

#include "../rpc_protocol_c.h"

/*! \brief TVM RPC channel write function.
 *
 * Tries to write `num_bytes` from `data` to the underlying channel.
 * \param data Pointer to data to write.
 * \param num_bytes Number of bytes avaiable in data.
 * \return The number of bytes written.
 */
typedef size_t (*tvm_rpc__channel_write)(void* context, uint8_t* data, size_t num_bytes);

/*! \brief circular buffer type.
 *
 * Used to buffer receive data from the incoming RPC channel.
 */
typedef struct {
  /*! \brief The underlying data buffer. */
  char* buf;

  /*! \brief The total number of bytes available to buf. Must be a power of 2. */
  size_t buf_capacity_bytes;

  /*! \brief Pointer to the first potentially-valid byte of data in the buffer. */
  char* head;

  /*! \brief Pointer to the next potentially-unused byte in the buffer. When head == tail, the
   *  buffer is empty.
   */
  char* tail;
} tvm_rpc__circular_buffer_t;

/*! \brief write to circular buffer */
size_t tvm_rpc__circular_buffer_write(tvm_rpc__circular_buffer_t* buf,
                                      uint8_t* data,
                                      size_t num_bytes);

/*! \brief return number of bytes in circular buffer */
size_t tvm_rpc__circular_buffer_size(tvm_rpc__circular_buffer_t* buf);

/*! \brief read from circular buffer */
size_t tvm_rpc__circular_buffer_read(tvm_rpc__circular_buffer_t* buf,
                                     uint8_t* out_data,
                                     size_t out_data_size_bytes);

typedef uint8_t tvm_rpc__server_state_t;

#define tvm_rpc__server_state_

typedef struct {
  tvm_rpc__circular_buffer_t* receive_buffer;

  uint8_t* send_buffer;
  size_t send_buffer_size_bytes;

  tvm_rpc_channel_write write_send_channel;

} tvm_rpc__server_t;

size_t tvm_rpc__server_receive(uint8_t* data, size_t data_num_bytes);

void tvm_rpc__server_loop(void);

#endif  // TVM_RUNTIME_RPC_MINRPC_C_MINRPC_SERVER_H_
