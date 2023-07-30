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
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "circular_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

    typedef struct
    {
        uint32_t event_id;
        uint32_t event_data_length;
        void* event_data;
    } event_t;

    typedef struct
    {
        void* buffer;
        uint32_t buffer_len;
        bool use_atomics;
        uint32_t alignment;
    } event_queue_config_t;

    typedef struct
    {
        event_queue_config_t config;
        circular_buffer_t _cb;
    } event_queue_t;

    /**
     * Initialize the event queue
     *
     * @param eq Event Queue
     * @param config Event Queue Configuration
    */
    bool EventQueueInit(event_queue_t* const eq, event_queue_config_t* const config);

    /**
     * Clear the event queue
     *
     * @param eq Event Queue
    */
    void EventQueueClear(event_queue_t* const eq);

    /**
     * Put an event off the event queue
     *
     * @param eq Event Queue
     * @param event_id Event identifier
     * @param event_data Data to accompany event
     * @param event_data_len Size of event data
    */
    bool EventQueuePut(event_queue_t* eq, const uint32_t event_id, void* const event_data, const uint32_t event_data_len);

    /**
     * Get an event off the event queue
     *
     * @param eq Event Queue
     * @return Pointer to the event - NULL if no event
    */
    event_t* EventQueueGet(event_queue_t* const eq);

    /**
     * Remove an event from the event queue
     *
     * @param eq Event Queue
    */
    void EventQueuePop(event_queue_t* const eq);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // EVENT_QUEUE_H
