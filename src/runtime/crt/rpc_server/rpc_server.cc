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
 * \file micro_rpc_server.cc
 * \brief MicroTVM RPC Server
 */

#include <inttypes.h>
#include <stdlib.h>

#include "dmlc/base.h"
#define DMLC_CMAKE_LITTLE_ENDIAN DMLC_IO_USE_LITTLE_ENDIAN
#include <tvm/runtime/c_runtime_api.h>
#include <tvm/runtime/crt/memory.h>
#include <tvm/runtime/micro/micro_rpc_server.h>

#include "crt_config.h"
#include "minrpc_server.h"

namespace tvm {
namespace runtime {

class Buffer {
 public:
  Buffer(uint8_t* data, size_t capacity) :
      data_{data}, capacity_{capacity}, head_{data}, tail_{data} {}

  size_t Write(const uint8_t* data, size_t data_size_bytes) {
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

  size_t Size() const {
    return tail_ - data_;
  }

  const uint8_t* Data() const {
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
  MicroIOHandler(utvm_rpc_channel_write_t send_func, void* send_func_ctx) :
      send_func_{send_func}, send_func_ctx_{send_func_ctx},
      receive_buffer_{receive_storage_, TVM_CRT_MAX_PACKET_SIZE_BYTES} {}

  size_t WriteFromHost(const uint8_t* data, size_t data_size_bytes) {
    return receive_buffer_.Write(data, data_size_bytes);
  }

  size_t PosixWrite(const uint8_t* buf, size_t buf_size_bytes) {
    return send_func_(send_func_ctx_, buf, buf_size_bytes);
  }

  ssize_t PosixRead(uint8_t* buf, size_t buf_size_bytes) {
    return receive_buffer_.Read(buf, buf_size_bytes);
  }

  void Close() {}

  void Exit(int code) {
    for (;;) ;
  }

  const Buffer& receive_buffer() const {
    return receive_buffer_;
  }

 private:
  utvm_rpc_channel_write_t send_func_;
  void* send_func_ctx_;

  uint8_t receive_storage_[TVM_CRT_MAX_PACKET_SIZE_BYTES];
  Buffer receive_buffer_;
};


class MicroRPCServer {
 public:
  MicroRPCServer(utvm_rpc_channel_write_t write_func, void* write_func_ctx) :
      io{write_func, write_func_ctx}, rpc_server{&io} {}

  MicroIOHandler io;
  MinRPCServer<MicroIOHandler> rpc_server;

  /*! \brief Process one packet from the receive buffer, if possible.
   *
   * \return true if additional packets could be processed. false if the server shutdown request has
   * been received.
   */
  bool Loop() {
    const Buffer& buf = io.receive_buffer();
    if (rpc_server.HasCompletePacket(buf.Data(), buf.Size())) {
      return rpc_server.ProcessOnePacket();
    }

    return true;
  }
};

}
}

extern "C" {

utvm_rpc_server_t utvm_rpc_server_init(utvm_rpc_channel_write_t write_func, void* write_func_ctx) {
  TVMInitializeRuntime();
  return static_cast<utvm_rpc_server_t>(
    new (vmalloc(sizeof(tvm::runtime::MicroRPCServer))) tvm::runtime::MicroRPCServer(
      write_func, write_func_ctx));
}

size_t utvm_rpc_server_receive_data(utvm_rpc_server_t server_ptr, const uint8_t* data, size_t data_size_bytes) {
  tvm::runtime::MicroRPCServer* server = static_cast<tvm::runtime::MicroRPCServer*>(server_ptr);
  return server->io.WriteFromHost(data, data_size_bytes);
}

void utvm_rpc_server_loop(utvm_rpc_server_t server_ptr) {
  tvm::runtime::MicroRPCServer* server = static_cast<tvm::runtime::MicroRPCServer*>(server_ptr);
  server->Loop();
}

}
