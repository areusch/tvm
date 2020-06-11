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
 * \file session.h
 * \brief RPC Session
 */

#ifndef TVM_RUNTIME_CRT_RPC_SERVER_SESSION_H_
#define TVM_RUNTIME_CRT_RPC_SERVER_SESSION_H_

#include <inttypes.h>
#include "buffer.h"
#include "framing.h"
#include "write_stream.h"

namespace tvm {
namespace runtime {

enum class PacketType : uint8_t {
  kStartSessionPacket = 0,
  kNormalTraffic = 1,
  kLogMessage = 2,
};

typedef struct SessionHeader {
  uint16_t session_id;
  PacketType packet_type;
} SessionHeader;

/*!
 * \brief CRT communication session management class.
 * Assumes the following properties provided by the underlying transport:
 *  - in-order delivery of packets.
 *  - reliable delivery of packets.
 *
 * Specifically, designed for use with UARTs. Will probably work over semihosting and USB; will
 * probably not work reliably enough over UDP.
 */
class Session {
 public:
  /*! \brief Callback invoked when a full packet is received.
   *
   * Note that this function is called for any packet with type other than kStartSessionPacket.
   */
  typedef void(*PacketReceivedFunc)(void*, PacketType, Buffer*);

  Session(uint8_t initial_session_nonce, Framer* framer,
          Buffer* receive_buffer,  PacketReceivedFunc packet_received_func,
          void* packet_received_func_context) :
      nonce_{initial_session_nonce}, state_{State::kReset}, session_id_{0}, receiver_{this},
      framer_{framer}, receive_buffer_{receive_buffer},
      packet_received_func_{packet_received_func},
      packet_received_func_context_{packet_received_func_context} {
        receive_buffer_->Clear();
      }

  /*!
   * \brief Start a new session regardless of state. Sends kStartSessionPacket.
   * \return 0 on success, negative error code on failure.
   */
  int StartSession();

  /*!
   * \brief Obtain receiver pointer to pass to the framing layer.
   * \return A WriteStream to which received data should be written. Owned by this class.
   */
  WriteStream* Receiver() {
    return &receiver_;
  }

  int SendPacket(PacketType packet_type, const uint8_t* packet_data, size_t packet_size_bytes);

  int StartPacket(PacketType packet_type, size_t packet_size_bytes);

  int SendPayloadChunk(const uint8_t* payload_data, size_t payload_size_bytes);

  int FinishPacket();

 private:
  class SessionReceiver : public WriteStream {
   public:
    SessionReceiver(Session* session) : session_{session} {}
    virtual ~SessionReceiver() {}

    ssize_t Write(const uint8_t* data, size_t data_size_bytes) override;
    void PacketDone(bool is_valid) override;

   private:
    Session* session_;
  };

  enum class State : uint8_t {
    kReset = 0,
    kStartSessionSent = 1,
    kSessionEstablished = 2,
  };

  void RegenerateNonce();

  int SendInternal(PacketType packet_type, const uint8_t* packet_data, size_t packet_size_bytes);

  void SendSessionStartReply(const SessionHeader& header);

  void ProcessStartSession(const SessionHeader& header);

  uint8_t nonce_;
  State state_;
  uint16_t session_id_;
  SessionReceiver receiver_;
  Framer* framer_;
  Buffer* receive_buffer_;
  PacketReceivedFunc packet_received_func_;
  void* packet_received_func_context_;
};

}  // namespace runtime
}  // namespace tvm

#endif  // TVM_RUNTIME_CRT_RPC_SERVER_SESSION_H_
