/* mbed Microcontroller Library
 * Copyright (c) 2015-2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef TVM_RUNTIME_CRT_RPC_SERVER_CIRCULAR_BUFFER_H_
#define TVM_RUNTIME_CRT_RPC_SERVER_CIRCULAR_BUFFER_H_

#include <stdint.h>
#include "platform/mbed_critical.h"
#include "platform/mbed_assert.h"

namespace tvm {
namespace runtime {

namespace {
/* Detect if CounterType of the Circular buffer is of unsigned type. */
template<typename T>
struct is_unsigned {
    static const bool value = false;
};
template<>
struct is_unsigned<unsigned char> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned short> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned int> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned long> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned long long> {
    static const bool value = true;
};
}

/**
 * \defgroup platform_CircularBuffer CircularBuffer functions
 * @{
 */

/** Templated Circular buffer class
 *
 *  @note Synchronization level: Interrupt safe
 *  @note CounterType must be unsigned and consistent with BufferSize
 */
template<typename T, uint32_t BufferSize, typename CounterType = uint32_t>
class CircularBuffer {
public:
    CircularBuffer() : _head(0), _tail(0), _full(false)
    {
        static_assert(
            is_unsigned<CounterType>::value,
            "CounterType must be unsigned"
        );

        static_assert(
            (sizeof(CounterType) >= sizeof(uint32_t)) ||
            (BufferSize < (((uint64_t) 1) << (sizeof(CounterType) * 8))),
            "Invalid BufferSize for the CounterType"
        );
    }

    ~CircularBuffer()
    {
    }

    /** Push the transaction to the buffer. This overwrites the buffer if it's
     *  full
     *
     * @param data Data to be pushed to the buffer
     */
    void push(const T &data)
    {
        TVMPlatformEnterCriticalSection();
        if (full()) {
            _tail++;
            if (_tail == BufferSize) {
                _tail = 0;
            }
        }
        _pool[_head++] = data;
        if (_head == BufferSize) {
            _head = 0;
        }
        if (_head == _tail) {
            _full = true;
        }
        TVMPlatformExitCriticalSection();
    }

    /** Pop the transaction from the buffer
     *
     * @param data Data to be popped from the buffer
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool pop(T &data)
    {
        bool data_popped = false;
        TVMPlatformEnterCriticalSection();
        if (!empty()) {
            data = _pool[_tail++];
            if (_tail == BufferSize) {
                _tail = 0;
            }
            _full = false;
            data_popped = true;
        }
        TVMPlatformExitCriticalSection();
        return data_popped;
    }

    /** Check if the buffer is empty
     *
     * @return True if the buffer is empty, false if not
     */
    bool empty() const
    {
        TVMPlatformEnterCriticalSection();
        bool is_empty = (_head == _tail) && !_full;
        TVMPlatformExitCriticalSection();
        return is_empty;
    }

    /** Check if the buffer is full
     *
     * @return True if the buffer is full, false if not
     */
    bool full() const
    {
        TVMPlatformEnterCriticalSection();
        bool full = _full;
        TVMPlatformExitCriticalSection();
        return full;
    }

    /** Reset the buffer
     *
     */
    void reset()
    {
        TVMPlatformEnterCriticalSection();
        _head = 0;
        _tail = 0;
        _full = false;
        TVMPlatformExitCriticalSection();
    }

    /** Get the number of elements currently stored in the circular_buffer */
    CounterType size() const
    {
        TVMPlatformEnterCriticalSection();
        CounterType elements;
        if (!_full) {
            if (_head < _tail) {
                elements = BufferSize + _head - _tail;
            } else {
                elements = _head - _tail;
            }
        } else {
            elements = BufferSize;
        }
        TVMPlatformExitCriticalSection();
        return elements;
    }

    /** Peek into circular buffer without popping
     *
     * @param data Data to be peeked from the buffer
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool peek(T &data) const
    {
        bool data_updated = false;
        TVMPlatformEnterCriticalSection();
        if (!empty()) {
            data = _pool[_tail];
            data_updated = true;
        }
        TVMPlatformExitCriticalSection();
        return data_updated;
    }

private:
    T _pool[BufferSize];
    CounterType _head;
    CounterType _tail;
    bool _full;
};

/**@}*/

}  // namespace runtime
}  // namespace tvm

#endif
