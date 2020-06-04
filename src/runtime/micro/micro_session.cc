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
 * \file micro_session.cc
 */

#include "micro_session.h"
#include <tvm/runtime/registry.h>

#include "../rpc/rpc_channel.h"
#include "../rpc/rpc_endpoint.h"
#include "../rpc/rpc_session.h"

namespace tvm {
namespace runtime {

TVM_REGISTER_GLOBAL("micro._rpc_connect").set_body([](TVMArgs args, TVMRetValue* rv) {
  std::unique_ptr<RPCChannel> channel(new CallbackChannel(args[1], args[2]));
  auto ep = RPCEndpoint::Create(std::move(channel), args[0], "");
  auto sess = CreateClientSession(ep);
  *rv = CreateRPCSessionModule(sess);
});

}  // namespace runtime
}  // namespace tvm
