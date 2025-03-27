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
#include <stdlib.h>
#include "event_queue.h"

bool EventQueueInit(event_queue_t* const eq, event_queue_config_t* const config)
{
    if (config->buffer == NULL)
        return false;
    if (config->buffer_len == 0)
        return false;
    memcpy(&eq->config, config, sizeof(event_queue_config_t));
    memset(config->buffer, 0, config->buffer_len);
    CircularBufferInit(&eq->_cb, config->buffer, config->buffer_len, config->use_atomics);
    return true;
}

void EventQueueClear(event_queue_t* const eq)
{
    CircularBufferClear(&eq->_cb);
}

bool EventQueuePut(event_queue_t* const eq, const event_id_t event_id, void* const event_data, const uint32_t event_data_len) {
    // If locking function present, lock
    if (eq->config.lock) {
        eq->config.lock();
    }

    // Get head point and amount of available free space
    uint32_t avail_space;
    void* head_ptr = CircularBufferHead(&eq->_cb, &avail_space);

    uint32_t q_item_size = sizeof(event_t) + event_data_len + sizeof(EVENT_MARKER);
    const uint32_t padding = (eq->config.alignment > 0) ? (q_item_size % eq->config.alignment) : 0U;
    q_item_size += padding;

    // No space
    if (avail_space < q_item_size)
        return false;

    // Check for contiguous space
    const uint32_t avail_contig_space = CircularBufferContiguousFreeSpace(&eq->_cb);

    // Check if there is enough available contiguous space for the event
    if (avail_contig_space < q_item_size) {
        if (avail_space - avail_contig_space < q_item_size) {
            // There is not enough contiguous space
            return false;
        }
        else {
            // There is enough space, but not enough contiguous space.
            // Add padding to wrap the head to the start of the buffer
            // to create more contiguous space.
            memset(head_ptr, PADDING, avail_contig_space);
            CircularBufferProduce(&eq->_cb, avail_contig_space);
            head_ptr = CircularBufferHead(&eq->_cb, &avail_space);
        }
    }

    // Place start of event marker and event and produce it ready for reading
    *(uint32_t*)head_ptr = EVENT_MARKER;
    ((uint32_t*)head_ptr) += sizeof(EVENT_MARKER);

    event_t* event_ptr = (event_t*)head_ptr;
    ((uint32_t*)head_ptr) += sizeof(event_t);
    
    event_ptr->event_id = event_id;
    event_ptr->event_data_length = event_data_len;
    event_ptr->event_data = head_ptr;
    memcpy(head_ptr, event_data, event_data_len);
    if (padding) {
        ((uint32_t*)head_ptr) += event_data_len;
        memset(head_ptr, PADDING, padding);
    }

    CircularBufferProduce(&eq->_cb, q_item_size);

    // If unlock function present, unlock
    if (eq->config.unlock) {
        eq->config.unlock();
    }

    return true;
}

event_t* EventQueueGet(event_queue_t* const eq)
{
    uint32_t available_bytes;
    void* tail = CircularBufferTail(&eq->_cb, &available_bytes);

    // Consume padding if present
    while (available_bytes > 0 && *(uint8_t*)tail == PADDING) {
        CircularBufferConsume(&eq->_cb, sizeof(PADDING));
        tail = CircularBufferTail(&eq->_cb, &available_bytes);
    }

    // No data
    if (available_bytes == 0) {
        return NULL;
    }

    // If there are bytes, it should be at least the size of an base event
    assert(available_bytes >= sizeof(event_t) + sizeof(EVENT_MARKER));

    // Provide pointer past the event marker
    ((uint32_t*)tail) += sizeof(EVENT_MARKER);

    return (event_t*)tail;
}

void EventQueuePop(event_queue_t* const eq)
{
    event_t* const evt = EventQueueGet(eq);
    if (evt != NULL) {
        CircularBufferConsume(&eq->_cb, sizeof(event_t) + evt->event_data_length + sizeof(EVENT_MARKER));
    }
}
