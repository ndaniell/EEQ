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
#include "event_queue.h"
#include <stdlib.h>

bool event_queue_init(event_queue_t *const eq,
                    event_queue_config_t *const config) {
  if (config->buffer == NULL)
    return false;
  if (config->buffer_len == 0)
    return false;
  memcpy(&eq->config, config, sizeof(event_queue_config_t));
  memset(config->buffer, 0, config->buffer_len);
  circular_buffer_init(&eq->_cb, config->buffer, config->buffer_len,
                     config->use_atomics);
  return true;
}

void event_queue_clear(event_queue_t *const eq) { circular_buffer_clear(&eq->_cb); }

bool event_queue_put(event_queue_t *const eq, const event_id_t event_id,
                   void *const event_data, const uint32_t event_data_len) {
  // If locking function present, lock
  if (eq->config.lock) {
    eq->config.lock();
  }

  // Get head point and amount of available free space
  uint32_t avail_space;
  void *head_ptr = circular_buffer_head(&eq->_cb, &avail_space);

  uint32_t q_item_size =
      sizeof(event_t) + event_data_len + sizeof(EVENT_MARKER);
  const uint32_t padding =
      (eq->config.alignment > 0) ? (q_item_size % eq->config.alignment) : 0U;
  q_item_size += padding;

  // No space
  if (avail_space < q_item_size)
    return false;

  // Check for contiguous space
  const uint32_t avail_contig_space =
      circular_buffer_contiguous_free_space(&eq->_cb);

  // Check if there is enough available contiguous space for the event
  if (avail_contig_space < q_item_size) {
    if (avail_space - avail_contig_space < q_item_size) {
      // There is not enough contiguous space
      return false;
    } else {
      // There is enough space, but not enough contiguous space.
      // Add padding to wrap the head to the start of the buffer
      // to create more contiguous space.
      memset(head_ptr, PADDING, avail_contig_space);
      circular_buffer_produce(&eq->_cb, avail_contig_space);
      head_ptr = circular_buffer_head(&eq->_cb, &avail_space);
    }
  }

  // Place start of event marker and event and produce it ready for reading
  *(uint32_t *)head_ptr = EVENT_MARKER;
  #ifdef _MSC_VER
    ((uint32_t *)head_ptr) += sizeof(EVENT_MARKER);
  #else
    head_ptr += sizeof(EVENT_MARKER);
  #endif

  event_t *event_ptr = (event_t *)head_ptr;
  #ifdef _MSC_VER
    ((uint32_t *)head_ptr) += sizeof(event_t);
  #else
    head_ptr += sizeof(event_t);
  #endif

  event_ptr->event_id = event_id;
  event_ptr->event_data_length = event_data_len;
  event_ptr->event_data = head_ptr;
  memcpy(head_ptr, event_data, event_data_len);
  if (padding) {
    #ifdef _MSC_VER
      ((uint32_t *)head_ptr) += event_data_len;
    #else
      head_ptr += event_data_len;
    #endif
    memset(head_ptr, PADDING, padding);
  }

  circular_buffer_produce(&eq->_cb, q_item_size);

  // If unlock function present, unlock
  if (eq->config.unlock) {
    eq->config.unlock();
  }

  return true;
}

event_t *event_queue_get(event_queue_t *const eq) {
  uint32_t available_bytes;
  void *tail = circular_buffer_tail(&eq->_cb, &available_bytes);

  // Consume padding if present
  while (available_bytes > 0 && *(uint8_t *)tail == PADDING) {
    circular_buffer_consume(&eq->_cb, sizeof(PADDING));
    tail = circular_buffer_tail(&eq->_cb, &available_bytes);
  }

  // No data
  if (available_bytes == 0) {
    return NULL;
  }

  // If there are bytes, it should be at least the size of an base event
  assert(available_bytes >= sizeof(event_t) + sizeof(EVENT_MARKER));

  // Provide pointer past the event marker
  #ifdef _MSC_VER
    ((uint32_t *)tail) += sizeof(EVENT_MARKER);
  #else
    tail += sizeof(EVENT_MARKER);
  #endif

  return (event_t *)tail;
}

void event_queue_pop(event_queue_t *const eq) {
  event_t *const evt = event_queue_get(eq);
  if (evt != NULL) {
    circular_buffer_consume(&eq->_cb, sizeof(event_t) + evt->event_data_length +
                                        sizeof(EVENT_MARKER));
  }
}
