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
 * \file micro_rpc_server.c
 * \brief MicroTVM RPC Server
 */

#include "micro_rpc_server.h"
#include <stdlib.h>
#include "minrpc_server.h"

namespace tvm {
namespace runtime {

class Buffer {
 public:
  Buffer(uint8_t* data, size_t capacity) :
      data_{data}, capacity_{capacity}, head_{data}, tail_{data} {}

  size_t Write(uint8_t* data, size_t data_size_bytes) {
    size_t num_bytes_available = capacity_ - Size();
    size_t num_bytes_to_copy = data_size_bytes;
    if (num_bytes_available < num_bytes_to_copy) {
      num_bytes_to_copy = num_bytes_available;
    }

    memcpy(tail_, data, num_bytes_to_copy);
    tail_ += num_bytes_to_copy;
    return num_bytes_to_copy;
  }

  size_t Read(uint8_t* data, size_t data_size_bytes) {
    if (head_ >= tail_) {
      return 0;
    }

    size_t num_bytes_to_copy = data_size_bytes;
    size_t num_bytes_available = tail_ - head_;
    if (num_bytes_available < num_bytes_to_copy) {
      num_bytes_to_copy = num_bytes_available;
    }

    memcpy(data, head_, num_bytes_to_copy);
    head_ += num_bytes_to_copy;
    return num_bytes_to_copy;
  }

  size_t Size() {
    return tail_ - data_;
  }

  uint8_t* Data() {
    return data_;
  }

  void Clear() {
    tail_ = data_;
    head_ = data_;
  }

 private:
  uint8_t* data_;
  size_t capacity_;
  uint8_t* head_;
  uint8_t* tail_;
};

class MicroIOHandler {
 public:
  MicroIOHandler(tvm_rpc_channel_write_t send_func) :
      send_func_{send_func}, rpc_server_{this} {}

  size_t WriteFromHost(const char* data, size_t data_size_bytes) {
    receive_buffer_.Write(data, data_size_bytes);
  }

  size_t PosixWrite(uint8_t* buf, size_t buf_size_bytes) {
    return send_func(buf, buf_size_bytes);
  }

  ssize_t PosixRead(uint8_t* buf, size_t buf_size_bytes) {
    return receive_buffer_.Read(buf, buf_size_bytes);
  }

  void Exit(int code) {
    for (;;) ;
  }

 private:
  tvm_rpc_channel_write_t send_func_;
  MinRPCServer<MicroIOHandler> rpc_server_;

  Buffer receive_buffer_;
}



extern "C" {

tvm_rpc_server_t utvm_rpc_server_init(tvm_rpc_channel_write_t write_func) {
  MicroIOHandler* handler = static_cast<MicroIOHandler>(vmalloc(sizeof(MicroIOHandler)));
  *handler = MicroIOHandler(write_func);

  return static_cast<tvm_rpc_server_t>(handler);
}

size_t utvm_rpc_server_receive_data(tvm_rpc_server_t server, const char* data, size_t data_size_bytes) {
  MicroIOHandler* handler = static_cast<MicroIOHandler>(server);
  return handler->Write(data, data_size_bytes);
}

void utvm_rpc_server_loop(tvm_rpc_server_t server) {
  MicroIOHandler* handler = static_cast<MicroIOHandler>(server);
  handler->Loop();
}

}
