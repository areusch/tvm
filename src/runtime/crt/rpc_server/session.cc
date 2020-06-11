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

#include "session.h"
#include <tvm/runtime/crt/logging.h>

namespace tvm {
namespace runtime {

void Session::RegenerateNonce() {
  nonce_ = (((nonce_ << 5) | (nonce_ >> 5)) + 1);

  if (nonce_ == 0) {
    nonce_++;
  }
}

int Session::SendInternal(PacketType packet_type, const uint8_t* packet_data, size_t packet_size_bytes) {
  int to_return = StartPacket(packet_type, packet_size_bytes);
  if (to_return != 0) {
    return to_return;
  }

  to_return = SendPayloadChunk(packet_data, packet_size_bytes);
  if (to_return != 0) {
    return to_return;
  }

  return framer_->FinishPacket();
}

int Session::StartPacket(PacketType packet_type, size_t packet_size_bytes) {
  SessionHeader header{session_id_, packet_type};
  int to_return = framer_->StartPacket(packet_size_bytes + sizeof(SessionHeader));
  if (to_return != 0) {
    return to_return;
  }

  return framer_->WritePayloadChunk(reinterpret_cast<uint8_t*>(&header), sizeof(SessionHeader));
}

int Session::SendPayloadChunk(const uint8_t* payload, size_t payload_size_bytes) {
  return framer_->WritePayloadChunk(payload, payload_size_bytes);
}

int Session::FinishPacket() {
  return framer_->FinishPacket();
}

int Session::StartSession() {
  RegenerateNonce();
  session_id_ = nonce_;
  int to_return = SendInternal(PacketType::kStartSessionPacket, nullptr, 0);
  if (to_return == 0) {
    state_ = State::kStartSessionSent;
  }

  return to_return;
}

int Session::SendPacket(PacketType packet_type, const uint8_t* packet_data, size_t packet_size_bytes) {
  if (state_ != State::kSessionEstablished) {
    return -1;
  }

  return SendInternal(packet_type, packet_data, packet_size_bytes);
}

ssize_t Session::SessionReceiver::Write(const uint8_t* data, size_t data_size_bytes) {
  size_t bytes_written = session_->receive_buffer_->Write(data, data_size_bytes);
  if (bytes_written != data_size_bytes) {
    return -1;
  }

  return bytes_written;
}

void Session::SessionReceiver::PacketDone(bool is_valid) {
  if (is_valid) {
    SessionHeader header;
    if (session_->receive_buffer_->Read(reinterpret_cast<uint8_t*>(&header), sizeof(header)) == sizeof(header)) {
      if (header.session_id == session_->session_id_) {
        switch (header.packet_type) {
        case PacketType::kStartSessionPacket:
          session_->ProcessStartSession(header);
          break;
        default:
          if (session_->state_ == State::kSessionEstablished) {
            session_->packet_received_func_(
              session_->packet_received_func_context_, header.packet_type, session_->receive_buffer_);
          }
          break;
        }
      } else if (header.session_id == 0 && header.packet_type == PacketType::kLogMessage) {
        // Special case for log messages: session id can be 0.
        session_->packet_received_func_(
          session_->packet_received_func_context_, header.packet_type, session_->receive_buffer_);
      }
    }
  }

  session_->receive_buffer_->Clear();
}

void Session::SendSessionStartReply(const SessionHeader& header) {
  RegenerateNonce();
  session_id_ = (header.session_id & 0xff) | (nonce_ << 8);
  int to_return = SendInternal(PacketType::kStartSessionPacket, nullptr, 0);
  CHECK(to_return == 0);
}

void Session::ProcessStartSession(const SessionHeader& header) {
  switch (state_) {
  case State::kReset:
    if ((header.session_id & 0xff) != 0 &&
        ((header.session_id >> 8) & 0xff) == 0) {
      SendSessionStartReply(header);
      state_ = State::kSessionEstablished;
    } else {
      CHECK(StartSession() == 0);
      state_ = State::kStartSessionSent;
    }
    break;

  case State::kStartSessionSent:
    if ((header.session_id & 0xff) == nonce_) {
      session_id_ = header.session_id;
      state_ = State::kSessionEstablished;
    } else {
      CHECK(StartSession() == 0);
      state_ = State::kStartSessionSent;
    }
    break;

  case State::kSessionEstablished:
    if (header.session_id != session_id_ &&
        ((header.session_id >> 8) & 0xff) == 0) {
      SendSessionStartReply(header);
    } else {
      state_ = State::kReset;
    }
    break;

  default:
    state_ = State::kReset;
  }
}

}  // namespace runtime
}  // namespace tvm
