/**
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
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event_queue.h"

#define BUFFER_SIZE (uint32_t)512

static event_queue_config_t default_config(void* buffer, uint32_t buffer_size) {
    event_queue_config_t eq_config = { .buffer = buffer,
                                       .buffer_len = buffer_size,
                                       .alignment = 4,
                                       .use_atomics = false,
                                       .lock = NULL,
                                       .unlock = NULL };
    return eq_config;
}

static void round_trip_test(event_queue_t* eq) {
    uint32_t event_id = 1;
    char* event_data = "Hello World";
    const uint32_t event_data_len = strlen(event_data) + 1;
    assert(event_queue_put(eq, 1, event_data, event_data_len) == true);

    event_t* out_event = event_queue_get(eq);
    assert(out_event != NULL);

    assert(out_event->event_id == event_id);
    assert(out_event->event_data_length == event_data_len);
    assert(memcmp(out_event->event_data, event_data, out_event->event_data_length) == 0);

    event_queue_pop(eq);
    assert(event_queue_get(eq) == NULL);
}

/**
 * Happy path example
*/
void test_happy_trails() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    event_queue_init(&eq, &eq_config);

    round_trip_test(&eq);
}

/**
  * Test zero alignment
 */
void test_zero_alignment() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    eq_config.alignment = 0;
    event_queue_init(&eq, &eq_config);

    round_trip_test(&eq);
}

/**
  * Test no atomics alignment
 */
void test_no_atomics() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    eq_config.use_atomics = false;
    event_queue_init(&eq, &eq_config);

    round_trip_test(&eq);
}

/**
 * Verify Empty Full Behavior
*/
void test_event_queue_full() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    event_queue_init(&eq, &eq_config);

    char* event_data = "Hello World";
    const uint32_t event_data_len = strlen(event_data) + 1;
    // Size of event, plus data size (including null), plus marker
    uint32_t q_item_size = sizeof(event_t) + event_data_len + sizeof(EVENT_MARKER);
    uint32_t event_queue_count = BUFFER_SIZE / q_item_size;

    // Fill the queue
    for (uint32_t i = 0; i < event_queue_count; i++) {
        assert(event_queue_put(&eq, 1, event_data, event_data_len) == true);
    }

    // Should be full
    assert(event_queue_put(&eq, 1, event_data, event_data_len) == false);

    // Pop twice to guarantee space
    event_queue_pop(&eq);
    event_queue_pop(&eq);

    // Should have space for a new event
    assert(event_queue_put(&eq, 1, event_data, event_data_len) == true);
}


/**
 * Verify Empty Queue Behavior
*/
void test_event_queue_empty() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    event_queue_init(&eq, &eq_config);

    assert(event_queue_get(&eq) == NULL);
}

/**
 * Verify events filled match events popped
*/
void test_event_queue_fill_and_empty() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    event_queue_init(&eq, &eq_config);

    // Size of event, plus data size (including null), plus marker
    uint32_t q_item_size = sizeof(event_t) + 1;
    uint32_t event_queue_count = BUFFER_SIZE / q_item_size;

    // Fill the queue
    for (uint32_t i = 0; i < event_queue_count; i++) {
        assert(event_queue_put(&eq, i, NULL, 0) == true);
    }

    // Should be full
    assert(event_queue_put(&eq, 1, NULL, 0) == false);

    // Empty the queue
    for (uint32_t i = 0; i < event_queue_count; i++) {
        event_t* out_event = event_queue_get(&eq);
        assert(out_event != NULL);
        assert(out_event->event_id == i);
        assert(out_event->event_data_length == 0);
        assert(out_event->event_data == NULL);
        event_queue_pop(&eq);
    }

    // Fill the queue back up
    for (uint32_t i = 0; i < event_queue_count; i++) {
        assert(event_queue_put(&eq, i, NULL, 0) == true);
    }

    // Should be full
    assert(event_queue_put(&eq, 1, NULL, 0) == false);
}

/**
 * Fuzz Test event data size
*/
void test_event_queue_fuzz_event_data_length() {
    uint8_t rand_buffer[BUFFER_SIZE];
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    event_queue_init(&eq, &eq_config);

    const uint32_t test_cycles = 1000;
    uint32_t put_count = 0;
    uint32_t get_count = 0;
    const uint32_t max_high_water = BUFFER_SIZE - (BUFFER_SIZE % (sizeof(event_t) + 1));  // +1 for marker
    for (uint32_t i = 0; i < test_cycles; i++) {
        event_queue_init(&eq, &eq_config);
        // Run event size combinations until the high water fill mark hit max
        while (eq._cb.high_water_fill_count <= max_high_water) {
            // Fill the queue with random sized events with random data
            bool placed = false;
            do {
                uint32_t event_data_size = rand() % (BUFFER_SIZE / 4);
                for (uint32_t j = 0; j < event_data_size; j++) {
                    rand_buffer[j] = (uint8_t)rand();
                }
                placed = event_queue_put(&eq, put_count, rand_buffer, event_data_size);
                if (placed) {
                    put_count++;
                }
            } while (placed);

            // Empty the queue
            event_t* out_event = NULL;
            do {
                out_event = event_queue_get(&eq);
                if (out_event) {
                    assert(out_event->event_id == get_count);
                    get_count++;
                    event_queue_pop(&eq);
                }
            } while (out_event);
        }
    }
}


static uint32_t test_lock_count = 0;
static uint32_t test_unlock_count = 0;

static void test_lock() {
    test_lock_count++;
}

static void test_unlock() {
    test_unlock_count++;
}

/**
 * Test option write lock function
*/
void test_lock_unlock() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    event_queue_config_t eq_config = default_config(buffer, BUFFER_SIZE);
    eq_config.lock = test_lock;
    eq_config.unlock = test_unlock;
    event_queue_init(&eq, &eq_config);

    test_lock_count = 0;
    test_unlock_count = 0;

    round_trip_test(&eq);

    assert(test_lock_count == 1);
    assert(test_unlock_count == 1);
}

/**
 * Main
*/
int main(int argc, char* argv[]) {
    srand(time(NULL));
    test_happy_trails();
    test_zero_alignment();
    test_no_atomics();
    test_event_queue_full();
    test_event_queue_empty();
    test_event_queue_fuzz_event_data_length();
    test_lock_unlock();
    return 0;
}