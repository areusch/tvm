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
 * \file runtime/crt/logging.h
 * \brief A replacement of the dmlc logging system that avoids
 *  the usage of GLOG and C++ headers
 */

#ifndef TVM_RUNTIME_CRT_LOGGING_H_
#define TVM_RUNTIME_CRT_LOGGING_H_

#include <tvm/runtime/crt/platform.h>

#ifdef TVM_RUNTIME_CRT_CONFIG_H_
#error "Must include logging.h before crt_config.h"
#endif

#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_ERROR 0

#include "crt_config.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_WARN
#endif

#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((format(printf,1,2))) TVMLogf(const char* fmt, ...);

#define LOG(level, x, ...)                      \
  if (LOG_LEVEL >= level) {                     \
    TVMLogf(x, ##__VA_ARGS__);                  \
  }

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(x, ...) TVMLogf(x, ##__VA_ARGS__)
#else
#define LOG_ERROR(x, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(x, ...) TVMLogf(x, ##__VA_ARGS__)
#else
#define LOG_WARN(x, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(x, ...) TVMLogf(x, ##__VA_ARGS__)
#else
#define LOG_INFO(x, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(x, ...) TVMLogf(x, ##__VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif

#ifndef CHECK
#define CHECK(x)                                 \
  do {                                           \
    if (!(x)) {                                  \
      LOG_ERROR("Check failed: %s\n", #x);     \
      TVMPlatformAbort(-1);                      \
    }                                            \
  } while (0)
#endif

#ifndef CHECK_BINARY_OP
#define CHECK_BINARY_OP(op, x, y, fmt, ...)                                             \
  do {                                                                                  \
    if (!(x op y)) {                                                                    \
      LOG_ERROR("Check failed: %s %s %s: " fmt "\n", #x, #op, #y, ##__VA_ARGS__); \
      exit(-1);                                                                         \
    }                                                                                   \
  } while (0)
#endif

#ifndef CHECK_LT
#define CHECK_LT(x, y, fmt, ...) CHECK_BINARY_OP(<, x, y, fmt, ##__VA_ARGS__)
#endif

#ifndef CHECK_GT
#define CHECK_GT(x, y, fmt, ...) CHECK_BINARY_OP(>, x, y, fmt, ##__VA_ARGS__)
#endif

#ifndef CHECK_LE
#define CHECK_LE(x, y, fmt, ...) CHECK_BINARY_OP(<=, x, y, fmt, ##__VA_ARGS__)
#endif

#ifndef CHECK_GE
#define CHECK_GE(x, y, fmt, ...) CHECK_BINARY_OP(>=, x, y, fmt, ##__VA_ARGS__)
#endif

#ifndef CHECK_EQ
#define CHECK_EQ(x, y, fmt, ...) CHECK_BINARY_OP(==, x, y, fmt, ##__VA_ARGS__)
#endif

#ifndef CHECK_NE
#define CHECK_NE(x, y, fmt, ...) CHECK_BINARY_OP(!=, x, y, fmt, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TVM_RUNTIME_CRT_LOGGING_H_
