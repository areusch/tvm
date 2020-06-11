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
 * \file framing.cc
 * \brief Framing for RPC.
 */

#include "framing.h"

#include <type_traits>
#include <string.h>
#include <tvm/runtime/crt/logging.h>

namespace tvm {
namespace runtime {

template <typename E>
static constexpr auto to_integral(E e) {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

int Unframer::Write(uint8_t* data, size_t data_size_bytes, size_t* bytes_consumed) {
  int return_code = 0;
  input_ = data;
  input_size_bytes_ = data_size_bytes;

  while (return_code == 0 && input_size_bytes_ > 0) {
    switch (state_) {
    case State::kFindPacketStart:
      return_code = FindPacketStart();
      break;
    case State::kFindPacketLength:
      return_code = FindPacketLength();
      break;
    case State::kFindPacketCrc:
      return_code = FindPacketCrc();
      break;
    case State::kFindCrcEnd:
      return_code = FindCrcEnd();
      break;
    default:
      return_code = -1;
      break;
    }
  }

  input_ = nullptr;
  input_size_bytes_ = 0;
  *bytes_consumed = data_size_bytes - input_size_bytes_;

  return return_code;
}

int Unframer::FindPacketStart() {
  size_t i;
  for (i = 0; i < input_size_bytes_; i++) {
    if (input_[i] == to_integral(Escape::kEscapeStart)) {
      saw_escape_start_ = !saw_escape_start_;
    } else if (input_[i] == to_integral(Escape::kPacketStart) && saw_escape_start_) {
      state_ = State::kFindPacketLength;
      break;
    }
  }

  input_ += i;
  input_size_bytes_ -= i;
  return 0;
}

int Unframer::ConsumeInput(uint8_t* buffer, size_t buffer_size_bytes, size_t* bytes_filled) {
  int to_return = 0;
  size_t i;
  *bytes_filled = 0;
  for (i = 0; i < input_size_bytes_; i++) {
    uint8_t c = input_[i];
    if (saw_escape_start_) {
      saw_escape_start_ = false;

      if (c == to_integral(Escape::kPacketStart)) {
        to_return = -1;
        // Rewind 1 byte, so that the next packet will parse in full. If saw_escape_start_ was set
        // during the previous Write() call, simulate the CRC.
        if (i > 0) {
          i--;
        } else {
          // Overwrite previous CRC value, since the one that was computed won't be needed.
          uint8_t escape_start = to_integral(Escape::kEscapeStart);
          crc_ = crc16_compute(&escape_start, 1, NULL);
        }
        break;
      } else if (c == to_integral(Escape::kEscapeNop)) {
        continue;
      }
    } else if (c == to_integral(Escape::kEscapeStart)) {
      saw_escape_start_ = true;
      continue;
    }

    buffer[*bytes_filled] = c;
    (*bytes_filled)++;
  }

  crc_ = crc16_compute(input_, i, &crc_);
  input_ += i;
  input_size_bytes_ -= i;
  return to_return;
}

int Unframer::AddToBuffer(size_t buffer_full_bytes) {
  CHECK(!is_buffer_full(buffer_full_bytes));
  return ConsumeInput(
    &buffer_[num_buffer_bytes_valid_], buffer_full_bytes - num_buffer_bytes_valid_,
    &num_buffer_bytes_valid_);
}

void Unframer::ClearBuffer() {
  num_buffer_bytes_valid_ = 0;
}

int Unframer::FindPacketLength() {
  size_t consumed = 0;
  int to_return = AddToBuffer(PacketFieldSizeBytes::kPayloadLength);
  if (to_return != 0) {
    return to_return;
  }

  if (!is_buffer_full(PacketFieldSizeBytes::kPayloadLength)) {
    return 0;
  }

  // TODO endian
  num_payload_bytes_remaining_ = *((uint32_t*) buffer_);
  ClearBuffer();
  state_ = State::kFindPacketCrc;
  return 0;
};


int Unframer::FindPacketCrc() {
  CHECK(num_buffer_bytes_valid_ == 0);

  while (num_payload_bytes_remaining_ > 0) {
    size_t num_bytes_to_buffer = num_buffer_bytes_valid_;
    if (num_bytes_to_buffer > sizeof(buffer_)) {
      num_bytes_to_buffer = sizeof(buffer_);
    }

    size_t prev_num_buffer_bytes_valid = num_buffer_bytes_valid_;
    {
      int to_return = AddToBuffer(num_buffer_bytes_valid_);
      if (to_return != 0) {
        return to_return;
      }
    }

    if (prev_num_buffer_bytes_valid == num_buffer_bytes_valid_) {
      // Return if no bytes were consumed from the input.
      return 0;
    }

    {
      int to_return = stream_->WriteAll(buffer_, num_buffer_bytes_valid_);
      if (to_return != 0) {
        return to_return;
      }
    }

    num_payload_bytes_remaining_ -= num_buffer_bytes_valid_;
    ClearBuffer();
  }

  if (num_payload_bytes_remaining_ == 0) {
    state_ = State::kFindCrcEnd;
  }

  return 0;
}

int Unframer::FindCrcEnd() {
  size_t consumed = 0;
  int to_return = AddToBuffer(PacketFieldSizeBytes::kCrc);
  if (to_return != 0) {
    return to_return;
  }

  if (!is_buffer_full(PacketFieldSizeBytes::kCrc)) {
    return 0;
  }

  // TODO endian
  stream_->PacketDone(crc_ == *((uint16_t*) buffer_));
  ClearBuffer();
  state_ = State::kFindPacketStart;
  return 0;
}


int Framer::Write(const uint8_t* payload, size_t payload_size_bytes) {
  int to_return;
  to_return = StartPacket(payload_size_bytes);
  if (to_return != 0) {
    return to_return;
  }

  to_return = WritePayloadChunk(payload, payload_size_bytes);
  if (to_return != 0) {
    return to_return;
  }

  to_return = FinishPacket();
  return to_return;
}

int Framer::StartPacket(size_t payload_size_bytes) {
  uint8_t packet_header[3 + sizeof(uint32_t)];
  size_t ptr = 0;
  if (state_ == State::kReset) {
    packet_header[ptr] = to_integral(Escape::kEscapeNop);
    ptr++;
  }
  packet_header[ptr] = to_integral(Escape::kEscapeStart);
  ptr++;
  packet_header[ptr] = to_integral(Escape::kPacketStart);
  ptr++;

  crc_ = 0xffff;
  memcpy((void*) &packet_header[ptr], &payload_size_bytes, sizeof(payload_size_bytes));
  return WriteAndCrc(packet_header, sizeof(packet_header));
}

int Framer::WriteAndCrc(const uint8_t* data, size_t data_size_bytes) {
  size_t buffer_size = data_size_bytes;
  if (data_size_bytes > kMaxStackBufferSizeBytes) {
    buffer_size = kMaxStackBufferSizeBytes;
  }

  while (data_size_bytes > 0) {
    uint8_t buffer[128];
    size_t buffer_ptr = 0;
    for (size_t i = 0; i < data_size_bytes; i++) {
      uint8_t c = data[i];
      if (c != to_integral(Escape::kEscapeStart)) {
        buffer[buffer_ptr] = c;
        buffer_ptr++;
        continue;
      }

      if (buffer_ptr == buffer_size - 1) {
        break;
      }

      buffer[buffer_ptr] = to_integral(Escape::kEscapeStart);
      buffer_ptr++;

      buffer[buffer_ptr] = to_integral(Escape::kEscapeStart);
      buffer_ptr++;
    }

    ssize_t to_return = stream_->WriteAll(buffer, buffer_ptr);
    if (to_return < 0) {
      return to_return;
    }

    crc_ = crc16_compute(buffer, buffer_ptr, &crc_);
  }

  return 0;
}

int Framer::WritePayloadChunk(const uint8_t* payload_chunk, size_t payload_chunk_size_bytes) {
  if (state_ != State::kTransmitPacketPayload ||
      payload_chunk_size_bytes > num_payload_bytes_remaining_) {
    return -1;
  }

  int to_return = WriteAndCrc(payload_chunk, payload_chunk_size_bytes);
  if (to_return != 0) {
    state_ = State::kReset;
    return to_return;
  }

  num_payload_bytes_remaining_ -= payload_chunk_size_bytes;
  return 0;
}

int Framer::FinishPacket() {
  int to_return = stream_->WriteAll(reinterpret_cast<uint8_t*>(&crc_), sizeof(crc_));
  if (to_return != 0) {
    state_ = State::kReset;
  } else {
    state_ = State::kIdle;
  }
  return to_return;
}

}  // namespace runtime
}  // namespace tvm
