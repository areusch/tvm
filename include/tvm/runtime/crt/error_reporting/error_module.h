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
 * \file tvm/runtime/crt/error_reporting/error_module.h
 * \brief Defines an error module used by RPC.
 */

#ifndef TVM_RUNTIME_CRT_ERROR_REPORTING_ERROR_MODULE_H_
#define TVM_RUNTIME_CRT_ERROR_REPORTING_ERROR_MODULE_H_

#include <inttypes.h>
#include <stdbool.h>

#define kErrorModuleMagicNumber 0xAA

#ifdef __cplusplus
extern "C" {
#endif

typedef enum error_source {
  kTVMPlatform = 0x00,
  kZephyr = 0x01,
} error_source_t;

typedef struct ErrorModule {
    uint8_t magic_num;
    error_source_t source;
    uint16_t reason;
    uint16_t crc;
} ErrorModule;


bool ErrorModuleIsValid(ErrorModule* error_ptr);
bool ErrorModuleIsCRCValid(ErrorModule* error_ptr);
void ErrorModuleSetError(ErrorModule* error_ptr, error_source_t source, uint16_t reason);
uint16_t ErrorModuleCalculateCRC(ErrorModule* error_ptr);

#ifdef __cplusplus
}
#endif

#endif // TVM_RUNTIME_CRT_ERROR_REPORTING_ERROR_MODULE_H_
