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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <tvm/runtime/crt/memory.h>

#include "../../src/runtime/crt/utvm_rpc_server/buffer.h"
#include "../../src/runtime/crt/utvm_rpc_server/session.h"

#include "buffer_write_stream.h"

#include "crt_config.h"

using ::tvm::runtime::Unframer;
using ::tvm::runtime::Framer;
using ::tvm::runtime::MessageType;
using ::tvm::runtime::Session;

extern "C" {
void TestSessionMessageReceivedThunk(void* context, MessageType message_type, Buffer* buf);
}

class ReceivedMessage {
 public:
  ReceivedMessage(MessageType type, std::string message) : type{type}, message{message} {}

  bool operator==(const ReceivedMessage& other) const {
    return other.type == type && other.message == message;
  }

  MessageType type;
  std::string message;
};

class TestSession {
 public:
  TestSession(uint8_t initial_nonce) : framer{&framer_write_stream},
                                       receive_buffer{receive_buffer_array, sizeof(receive_buffer_array)},
                                       sess{initial_nonce,
                                            &framer,
                                            &receive_buffer,
                                            TestSessionMessageReceivedThunk,
                                            this},
                                       unframer{sess.Receiver()} {}

  template <unsigned int N>
  void ExpectFramedPacket(const char (&expected)[N]) {
    EXPECT_EQ(std::string(expected, N - 1), framer_write_stream.BufferContents());
  }

  void WriteTo(TestSession* other) {
    auto framer_buffer = framer_write_stream.BufferContents();
    size_t bytes_to_write = framer_buffer.size();
    const uint8_t* write_cursor = reinterpret_cast<const uint8_t*>(framer_buffer.data());
    while (bytes_to_write > 0) {
      size_t bytes_consumed;
      auto to_return = other->unframer.Write(write_cursor, bytes_to_write, &bytes_consumed);
      EXPECT_EQ(to_return, kTvmErrorNoError);
      bytes_to_write -= bytes_consumed;
      write_cursor += bytes_consumed;
    }
  }

  void ClearBuffers() {
    framer_write_stream.Reset();
    messages_received.clear();
    sess.ClearReceiveBuffer();
  }

  std::vector<ReceivedMessage> messages_received;
  BufferWriteStream<300> framer_write_stream;
  Framer framer;
  uint8_t receive_buffer_array[300];
  Buffer receive_buffer;
  Session sess;
  Unframer unframer;
};

extern "C" {
void TestSessionMessageReceivedThunk(void* context, MessageType message_type, Buffer* buf) {
  std::string message;
  if (message_type != MessageType::kStartSessionMessage) {
    uint8_t message_buf[300];
    EXPECT_LE(buf->ReadAvailable(), sizeof(message_buf));
    size_t message_size_bytes = buf->Read(message_buf, sizeof(message_buf));
    message = std::string(reinterpret_cast<char*>(message_buf), message_size_bytes);
  }

  static_cast<TestSession*>(context)->messages_received.emplace_back(
      ReceivedMessage(message_type, message));
}
}

void PrintTo(tvm_crt_error_t p, std::ostream* os) {
  std::ios_base::fmtflags f(os->flags());
  *os << "tvm_crt_error_t(0x" << std::hex << std::setw(8) << std::setfill('0') << p << ")";
  os->flags(f);
}

void PrintTo(ReceivedMessage msg, std::ostream* os) {
  *os << "ReceivedMessage(" << int(msg.type) << ", \"" << msg.message << "\")";
}

class SessionTest : public ::testing::Test {
 public:
  static constexpr const uint8_t kAliceNonce = 0x3c;
  static constexpr const uint8_t kBobNonce = 0xab;

  TestSession alice_{kAliceNonce};
  TestSession bob_{kBobNonce};
};

TEST_F(SessionTest, NormalExchange) {
  alice_.ClearBuffers();
  tvm_crt_error_t err = alice_.sess.StartSession();
  EXPECT_EQ(err, kTvmErrorNoError);
  alice_.ExpectFramedPacket("\xfe\xff\xfd\x03\0\0\0\x82\0\0\x1E\x2");

  bob_.ClearBuffers();
  alice_.WriteTo(&bob_);
  bob_.ExpectFramedPacket("\xfe\xff\xfd\x3\0\0\0f\x82\0\x15\x03");
  EXPECT_TRUE(bob_.sess.IsEstablished());

  bob_.WriteTo(&alice_);
  EXPECT_TRUE(alice_.sess.IsEstablished());
  ASSERT_EQ(alice_.messages_received.size(), 1);
  EXPECT_EQ(alice_.messages_received[0], ReceivedMessage(MessageType::kStartSessionMessage, ""));

  alice_.ClearBuffers();
  alice_.sess.SendMessage(MessageType::kNormalTraffic, reinterpret_cast<const uint8_t*>("hello"), 5);
  alice_.ExpectFramedPacket("\xFF\xFD\b\0\0\0\x82" "f\x10hello\x90(");
  alice_.WriteTo(&bob_);
  ASSERT_EQ(bob_.messages_received.size(), 2);
  EXPECT_EQ(bob_.messages_received[0], ReceivedMessage(MessageType::kStartSessionMessage, ""));
  EXPECT_EQ(bob_.messages_received[1], ReceivedMessage(MessageType::kNormalTraffic, "hello"));

  bob_.ClearBuffers();
  bob_.sess.SendMessage(MessageType::kNormalTraffic, reinterpret_cast<const uint8_t*>("olleh"), 5);
  bob_.ExpectFramedPacket("\xFF\xFD\b\0\0\0f\x82\x10olleh=\xd0");
  bob_.WriteTo(&alice_);
  ASSERT_EQ(alice_.messages_received.size(), 1);
  EXPECT_EQ(alice_.messages_received[0], ReceivedMessage(MessageType::kNormalTraffic, "olleh"));

  alice_.ClearBuffers();
  bob_.ClearBuffers();

  alice_.sess.SendMessage(MessageType::kLogMessage, reinterpret_cast<const uint8_t*>("log1"), 4);
  alice_.ExpectFramedPacket("\xff\xfd\a\0\0\0\x82" "f\x01log1\x90\x89");
  alice_.WriteTo(&bob_);
  ASSERT_EQ(bob_.messages_received.size(), 1);
  EXPECT_EQ(bob_.messages_received[0], ReceivedMessage(MessageType::kLogMessage, "log1"));

  bob_.sess.SendMessage(MessageType::kLogMessage, reinterpret_cast<const uint8_t*>("zero"), 4);
  bob_.ExpectFramedPacket("\xff\xfd\a\0\0\0f\x82\x1zerona");
  bob_.WriteTo(&alice_);
  ASSERT_EQ(alice_.messages_received.size(), 1);
  EXPECT_EQ(alice_.messages_received[0], ReceivedMessage(MessageType::kLogMessage, "zero"));
}

TEST_F(SessionTest, LogBeforeSessionStart) {
  alice_.sess.SendMessage(MessageType::kLogMessage, reinterpret_cast<const uint8_t*>("log1"), 4);
  alice_.ExpectFramedPacket("\xfe\xff\xfd\a\0\0\0\0\0\x1log1s\x90");
  alice_.WriteTo(&bob_);
  ASSERT_EQ(bob_.messages_received.size(), 1);
  EXPECT_EQ(bob_.messages_received[0], ReceivedMessage(MessageType::kLogMessage, "log1"));

  bob_.sess.SendMessage(MessageType::kLogMessage, reinterpret_cast<const uint8_t*>("zero"), 4);
  bob_.ExpectFramedPacket("\xfe\xff\xfd\a\0\0\0\0\0\x1zero1,");
  bob_.WriteTo(&alice_);
  ASSERT_EQ(alice_.messages_received.size(), 1);
  EXPECT_EQ(alice_.messages_received[0], ReceivedMessage(MessageType::kLogMessage, "zero"));
}

static constexpr const char kBobStartPacket[] = "\xfe\xff\xfd\x03\0\0\0f\0\0\xef~";

TEST_F(SessionTest, DoubleStart) {
  EXPECT_EQ(kTvmErrorNoError, alice_.sess.StartSession());
  alice_.ExpectFramedPacket("\xfe\xff\xfd\x03\0\0\0\x82\0\0\x1E\x2");
  EXPECT_FALSE(alice_.sess.IsEstablished());

  EXPECT_EQ(kTvmErrorNoError, bob_.sess.StartSession());
  bob_.ExpectFramedPacket(kBobStartPacket);
  EXPECT_FALSE(bob_.sess.IsEstablished());

  // Sending Alice -> Bob should have no effect (regenerated Bob nonce > regenerated Alice nonce).
  bob_.framer_write_stream.Reset();
  alice_.WriteTo(&bob_);
  bob_.ExpectFramedPacket("");
  EXPECT_FALSE(bob_.sess.IsEstablished());

  // Sending Bob -> Alice should start the session.
  alice_.ClearBuffers();
  size_t bytes_consumed;
  EXPECT_EQ(kTvmErrorNoError, alice_.unframer.Write(
              reinterpret_cast<const uint8_t*>(kBobStartPacket),
              sizeof(kBobStartPacket),
              &bytes_consumed));
  EXPECT_EQ(bytes_consumed, sizeof(kBobStartPacket));
  alice_.ExpectFramedPacket("\xFF\xFD\x3\0\0\0Ef\0\xF5\0");
  EXPECT_TRUE(alice_.sess.IsEstablished());

  bob_.ClearBuffers();
  alice_.WriteTo(&bob_);
  EXPECT_TRUE(bob_.sess.IsEstablished());
}

extern "C" {
void TVMPlatformAbort(int error_code) { FAIL() << "TVMPlatformAbort(" << error_code << ")"; }

void* TVMSystemLibEntryPoint() { return NULL; }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}
