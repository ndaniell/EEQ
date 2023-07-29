#include <stdio.h>
#include <string.h>
#include "event_queue.h"

#define BUFFER_SIZE (uint32_t)1024


void test_happy_trails() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    EventQueueInit(&eq, buffer, BUFFER_SIZE);

    char* event_data = "Hello World";
    event_t event = { .event_id = 1, event.event_data_length = strlen(event_data) + 1, .event_data = event_data };
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

void test_event_queue_full() {
    uint8_t buffer[BUFFER_SIZE];
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
}

void test_event_queue_empty() {
    uint8_t buffer[BUFFER_SIZE];
    event_queue_t eq;
    EventQueueInit(&eq, buffer, BUFFER_SIZE);

    assert(EventQueueGet(&eq) == NULL);
}


int main(int argc, char* argv[]) {
    test_happy_trails();
    test_event_queue_full();
    test_event_queue_empty();
    return 0;
}