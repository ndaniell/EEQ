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
#include <stdio.h>
#include <string.h>
#include "event_queue.h"

#define BUFFER_SIZE (uint32_t)64

 /**
  * Happy path example
 */
void test_happy_trails() {
    static uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    EventQueueInit(&eq, buffer, BUFFER_SIZE);

    char* event_data = "Hello World";
    event_t event = { .event_id = 1, .event_data_length = strlen(event_data) + 1, .event_data = event_data };
    assert(EventQueuePut(&eq, &event) == true);

    event_t* out_event = EventQueueGet(&eq);
    assert(out_event != NULL);

    assert(out_event->event_id == event.event_id);
    assert(out_event->event_data_length == event.event_data_length);
    assert(out_event->event_data == event.event_data);
    assert(memcmp(out_event->event_data, event.event_data, out_event->event_data_length) == 0);

    EventQueuePop(&eq);
    assert(EventQueueGet(&eq) == NULL);
}

/**
 * Verify Empty Full Behavior
*/
void test_event_queue_full() {
    static uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    EventQueueInit(&eq, buffer, BUFFER_SIZE);

    char* event_data = "Hello World";
    event_t event = { .event_id = 1, event.event_data_length = strlen(event_data) + 1, .event_data = event_data };
    uint32_t total_event_size = sizeof(event_t) + event.event_data_length;
    uint32_t event_queue_count = BUFFER_SIZE / total_event_size;
    for (uint32_t i = 0; i < event_queue_count; i++) {
        assert(EventQueuePut(&eq, &event) == true);
    }
    assert(EventQueuePut(&eq, &event) == false);
    EventQueueClear(&eq);
    //assert(EventQueuePut(&eq, &event) == true);
}


/**
 * Verify Empty Queue Behavior
*/
void test_event_queue_empty() {
    static uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    EventQueueInit(&eq, buffer, BUFFER_SIZE);

    assert(EventQueueGet(&eq) == NULL);
}

/**
 * Main
*/
int main(int argc, char* argv[]) {
    test_happy_trails();
    test_event_queue_full();
    test_event_queue_empty();
    return 0;
}