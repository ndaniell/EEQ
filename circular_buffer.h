/**
 * Copyright (c) 2025 Nicholas Daniell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic_int atomic_int_t;
#define atomicFetchAdd(a, b) std::atomic_fetch_add(a, b)
#else
#if defined(_MSC_VER)
#include "win32_stdatomic.h"
#else
#include <stdatomic.h>
#endif
typedef atomic_int atomic_int_t;
#define atomicFetchAdd(a, b) atomic_fetch_add(a, b)
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
  void *buffer;                     // Pointer to memory
  uint32_t length;                  // Size of memory
  uint32_t tail;                    // tail index
  uint32_t head;                    // head index
  volatile atomic_int_t fill_count; // number of used indexes
  bool atomic;                      // Enable or disable atomic operators
  uint32_t high_water_fill_count;   // Largest value the fill count has reached
} circular_buffer_t;

static inline void *circular_buffer_tail(circular_buffer_t *cb,
                                         uint32_t *available_bytes);

/**
 * Initialize the Circular Buffer
 *
 * @param cb Circular buffer
 * @param buffer Block of memory to be used for the circular buffer
 * @param length Size of the memory block
 * @param length use atomic operations
 */
static inline bool circular_buffer_init(circular_buffer_t *const cb,
                                        void *const buffer,
                                        const uint32_t length,
                                        const bool use_atomics) {
  cb->buffer = buffer;
  cb->length = length;
  cb->fill_count = 0;
  cb->head = cb->tail = 0;
  cb->atomic = use_atomics;
  cb->high_water_fill_count = 0;
  return true;
}

/**
 * Enables atomic functions when producing or consuming
 *
 * @param cb Circular buffer
 * @param atomic Use atomic operations
 */
static inline void circular_buffer_set_atomic(circular_buffer_t *const cb,
                                              const bool atomic) {
  cb->atomic = atomic;
}

/**
 * Reading (consuming) - Access end of buffer
 *
 *  This gives you a pointer to the end of the buffer, ready
 *  for reading, and the number of available bytes to read.
 *
 * @param cb Circular buffer
 * @param available_bytes On output, the number of bytes ready for reading
 * @return Pointer to the first bytes ready for reading, or NULL if buffer is
 * empty
 */
static inline void *circular_buffer_tail(circular_buffer_t *const cb,
                                         uint32_t *available_bytes) {
  *available_bytes = cb->fill_count;
  if (*available_bytes == 0)
    return NULL;
  return (void *)((char *)cb->buffer + cb->tail);
}

/**
 * Reading (consuming) - Consume bytes in buffer
 *
 *  This frees up the just-read bytes, ready for writing again.
 *
 * @param cb Circular buffer
 * @param amount Number of bytes to consume
 */
static inline void circular_buffer_consume(circular_buffer_t *const cb,
                                           const uint32_t amount) {
  cb->tail = (cb->tail + amount) % cb->length;
  if (cb->atomic) {
    atomicFetchAdd(&cb->fill_count, -(int)amount);
  } else {
    cb->fill_count -= amount;
  }
  assert(cb->fill_count >= 0);
  if (cb->fill_count > (int)cb->high_water_fill_count) {
    cb->high_water_fill_count = cb->fill_count;
  }
}

/**
 * Reading (consuming) - Access front of buffer
 *
 *  This gives you a pointer to the front of the buffer, ready
 *  for writing, and the number of available bytes to write.
 *
 * @param cb Circular buffer
 * @param available_bytes On output, the number of bytes ready for writing
 * @return Pointer to the first bytes ready for writing, or NULL if buffer is
 * full
 */
static inline void *circular_buffer_head(circular_buffer_t *const cb,
                                         uint32_t *available_bytes) {
  *available_bytes = (cb->length - cb->fill_count);
  if (*available_bytes == 0)
    return NULL;
  return (void *)((char *)cb->buffer + cb->head);
}

/**
 * Empties the circular buffer
 *
 * @param cb Circular buffer
 */
static inline void circular_buffer_clear(circular_buffer_t *const cb) {
  uint32_t fill_count;
  if (circular_buffer_tail(cb, &fill_count)) {
    circular_buffer_consume(cb, fill_count);
  }
}

/**
 * Writing (producing) - Produce bytes in buffer
 *
 *  This marks the given section of the buffer ready for reading.
 *
 * @param cb Circular buffer
 * @param amount Number of bytes to produce
 */
static inline void circular_buffer_produce(circular_buffer_t *const cb,
                                           uint32_t amount) {
  cb->head = (cb->head + amount) % cb->length;
  if (cb->atomic) {
    atomicFetchAdd(&cb->fill_count, (int)amount);
  } else {
    cb->fill_count += amount;
  }
  assert(cb->fill_count <= (int)cb->length);
}

/**
 * Writing (producing) - Helper routine to copy bytes to buffer
 *
 *  This copies the given bytes to the buffer, and marks them ready for reading.
 *
 * @param cb Circular buffer
 * @param src Source buffer
 * @param len Number of bytes in source buffer
 * @return true if bytes copied, false if there was insufficient space
 */
static inline bool circular_buffer_produce_bytes(circular_buffer_t *const cb,
                                                 void *const src,
                                                 const uint32_t len) {
  uint32_t space;
  void *ptr = circular_buffer_head(cb, &space);

  // No space
  if (space < len)
    return false;

  memcpy(ptr, src, len);
  circular_buffer_produce(cb, len);

  return true;
}

/**
 * Writing (producing) - Get the amount of free contiguous space after the head
 * pointer
 *
 * @param cb Circular buffer
 * @return number of contiguous bytes from the head
 */
static inline uint32_t
circular_buffer_contiguous_free_space(circular_buffer_t *const cb) {
  return cb->length - cb->head;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CIRCULAR_sBUFFER_H
