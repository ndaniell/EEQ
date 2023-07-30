# EEQ
[![ci](https://github.com/ndaniell/EEQ/actions/workflows/ci.yml/badge.svg)](https://github.com/ndaniell/EEQ/actions/workflows/ci.yml)

Embedded Event Queue

This library provides a C implementation of a Event Queue. An Event is a message style structure, represented by an identifier followed by accompanying data. The size of the event data can be dynamic. To avoid memory fragmentation associated with heap allocation and to minimize the memory footprint the library utilizes a circular buffer backend. The library ensure contiguous memory for events and allows the user to specify memory alignment for events. This is done using padding within the circular buffer which is consumed during a get. Memory is provided during initialization. The library leverages atomics to achieve a thread safe solution using single producer single consumer, but can be disabled. If multiple producers is desired a lock and unlock function can be provided via configuration to provide a write lock during a put.

## Initialize
```c
uint8_t buffer[BUFFER_SIZE];
event_queue_t eq;
event_queue_config_t eq_config = { .buffer = buffer, 
                                   .buffer_len = BUFFER_SIZE, 
                                   .alignment = 4, 
                                   .use_atomics = true
                                   .lock = NULL,
                                   .unlock = NULL };
EventQueueInit(&eq, &eq_config);
```

## Put
```c
uint32_t event_id = 1;
char* event_data = "Hello World";
const uint32_t event_data_len = strlen(event_data) + 1;  // Plus 1 for NULL
bool enqueued = EventQueuePut(&eq, event_id, event_data, event_data_len);
if (!enqueued) {
    // Queue full
}
```

## Get
```c
event_t* out_event = EventQueueGet(&eq);
if (out_event != NULL) {
    // consume event
    EventQueuePop(&eq);  // Remove event from event queue
}
```


    