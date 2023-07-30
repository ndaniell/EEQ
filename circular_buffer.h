/*
 * Copyright (c) 2023 Nicholas Daniell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C++"
{
#include <atomic>
    typedef std::atomic_int atomic_int_t;
#define atomicFetchAdd(a, b) std::atomic_fetch_add(a, b)
}
#else
#include <stdatomic.h>
typedef atomic_int atomic_int_t;
#define atomicFetchAdd(a, b) atomic_fetch_add(a, b)
#endif  // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

    typedef struct
    {
        void* buffer;                      // Pointer to memory
        uint32_t length;                   // Size of memory
        uint32_t tail;                     // tail index
        uint32_t head;                     // head index
        volatile atomic_int_t fill_count;  // number of used indexes
        bool atomic;                       // Enable or disable atomic operators
    } circular_buffer_t;

    static inline void* CircularBufferTail(circular_buffer_t* cbuffer, uint32_t* available_bytes);

    /**
     * Initialize the Circular Buffer
     *
     * @param cbuffer Circular buffer
     * @param buffer Block of memory to be used for the circular buffer
     * @param length Size of the memory block
     * @param length use atomic operations
     */
    static inline bool CircularBufferInit(circular_buffer_t* cbuffer, void* buffer, uint32_t length, bool use_atomics)
    {
        cbuffer->buffer = buffer;
        cbuffer->length = length;
        cbuffer->fill_count = 0;
        cbuffer->head = cbuffer->tail = 0;
        cbuffer->atomic = use_atomics;
        return true;
    }

    /**
     * Enables atomic functions when producing or consuming
     *
     * @param cbuffer Circular buffer
     * @param atomic Use atomic operations
     */
    static inline void CircularBufferSetAtomic(circular_buffer_t* cbuffer, bool atomic)
    {
        cbuffer->atomic = atomic;
    }

    /**
     * Reading (consuming) - Access end of buffer
     *
     *  This gives you a pointer to the end of the buffer, ready
     *  for reading, and the number of available bytes to read.
     *
     * @param cbuffer Circular buffer
     * @param available_bytes On output, the number of bytes ready for reading
     * @return Pointer to the first bytes ready for reading, or NULL if buffer is empty
     */
    static inline void* CircularBufferTail(circular_buffer_t* cbuffer, uint32_t* available_bytes)
    {
        *available_bytes = cbuffer->fill_count;
        if (*available_bytes == 0)
            return NULL;
        return (void*)((char*)cbuffer->buffer + cbuffer->tail);
    }

    /**
     * Reading (consuming) - Consume bytes in buffer
     *
     *  This frees up the just-read bytes, ready for writing again.
     *
     * @param cbuffer Circular buffer
     * @param amount Number of bytes to consume
     */
    static inline void CircularBufferConsume(circular_buffer_t* cbuffer, uint32_t amount)
    {
        cbuffer->tail = (cbuffer->tail + amount) % cbuffer->length;
        if (cbuffer->atomic)
        {
            atomicFetchAdd(&cbuffer->fill_count, -(int)amount);
        }
        else
        {
            cbuffer->fill_count -= amount;
        }
        assert(cbuffer->fill_count >= 0);
    }

    /**
     * Reading (consuming) - Access front of buffer
     *
     *  This gives you a pointer to the front of the buffer, ready
     *  for writing, and the number of available bytes to write.
     *
     * @param cbuffer Circular buffer
     * @param available_bytes On output, the number of bytes ready for writing
     * @return Pointer to the first bytes ready for writing, or NULL if buffer is full
     */
    static inline void* CircularBufferHead(circular_buffer_t* cbuffer, uint32_t* available_bytes)
    {
        *available_bytes = (cbuffer->length - cbuffer->fill_count);
        if (*available_bytes == 0)
            return NULL;
        return (void*)((char*)cbuffer->buffer + cbuffer->head);
    }

    /**
     * Empties the circular buffer
     *
     * @param cbuffer Circular buffer
     */
    static inline void CircularBufferClear(circular_buffer_t* cbuffer)
    {
        uint32_t fill_count;
        if (CircularBufferTail(cbuffer, &fill_count))
        {
            CircularBufferConsume(cbuffer, fill_count);
        }
    }

    /**
     * Writing (producing) - Produce bytes in buffer
     *
     *  This marks the given section of the buffer ready for reading.
     *
     * @param cbuffer Circular buffer
     * @param amount Number of bytes to produce
     */
    static inline void CircularBufferProduce(circular_buffer_t* cbuffer, uint32_t amount) {
        cbuffer->head = (cbuffer->head + amount) % cbuffer->length;
        if (cbuffer->atomic) {
            atomicFetchAdd(&cbuffer->fill_count, (int)amount);
        }
        else {
            cbuffer->fill_count += amount;
        }
        assert(cbuffer->fill_count <= cbuffer->length);
    }

    /**
     * Writing (producing) - Helper routine to copy bytes to buffer
     *
     *  This copies the given bytes to the buffer, and marks them ready for reading.
     *
     * @param cbuffer Circular buffer
     * @param src Source buffer
     * @param len Number of bytes in source buffer
     * @return true if bytes copied, false if there was insufficient space
     */
    static inline bool CircularBufferProduceBytes(circular_buffer_t* cbuffer, const void* src, uint32_t len) {
        uint32_t space;
        void* ptr = CircularBufferHead(cbuffer, &space);

        // No space
        if (space < len)
            return false;

        // No contiguous space to end, pad
        if (cbuffer->head + len > cbuffer->length) {
            uint32_t pad_count = cbuffer->length - cbuffer->head;
            memset(ptr, 0, pad_count);
            CircularBufferProduce(cbuffer, pad_count);
            ptr = CircularBufferHead(cbuffer, &space);
        }

        memcpy(ptr, src, len);
        CircularBufferProduce(cbuffer, len);

        return true;
    }

    static inline uint32_t CircularBufferContiguousFreeSpace(circular_buffer_t* cbuffer) {
        return cbuffer->length - cbuffer->head;
    }

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // CIRCULAR_BUFFER_H
