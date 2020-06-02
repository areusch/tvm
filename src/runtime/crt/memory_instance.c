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
 * \file memory_instance.c
 * \brief Virtual memory manager instantiation
 *
 * This file contains the global variables that back the memory pool. You can replace this in your
 * own projects with similar logic.
 */

#include "crt_config.h"
#include <inttypes.h>
#include <tvm/runtime/crt/memory.h>

/*! \brief Translate log memory size into bytes */
#define TVM_CRT_VIRT_MEM_SIZE ((1 << TVM_CRT_PAGE_BITS) * TVM_CRT_MAX_PAGES)

/**
 * \brief Memory pool for virtual dynamic memory allocation
 */
static uint8_t g_memory_pool[TVM_CRT_VIRT_MEM_SIZE];

MemoryManager* InstantiateGlobalMemoryManger(void) {
  return MemoryManagerCreate(g_memory_pool, TVM_CRT_VIRT_MEM_SIZE, TVM_CRT_PAGE_BITS);
}
