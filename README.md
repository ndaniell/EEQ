# Embedded Event Queue (EEQ)
[![ci](https://github.com/ndaniell/EEQ/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ndaniell/EEQ/actions/workflows/ci.yml)

This library offers a C implementation for an Event Queue, where an Event is defined as a message-style structure comprising an identifier and associated data. The size of this event data can vary dynamically. To reduce memory fragmentation from heap allocation and minimize the memory footprint, the library employs a circular buffer backend. It ensures contiguous memory allocation for events and permits user-specified memory alignment through padding within the circular buffer, utilized during retrieval. Memory allocation occurs during initialization. For thread safety, the library utilizes atomics to support a single producer-single consumer model, which can be optionally disabled. For scenarios requiring multiple producers, the library can be configured to include lock and unlock functions, ensuring a write lock during data insertion.

## Initialize
```c
uint8_t buffer[BUFFER_SIZE];
event_queue_t eq;
event_queue_config_t eq_config = { .buffer = buffer, 
                                   .buffer_len = BUFFER_SIZE, 
                                   .alignment = 4, 
                                   .use_atomics = true,
                                   .lock = NULL,
                                   .unlock = NULL };
event_queue_init(&eq, &eq_config);
```

## Put
```c
uint32_t event_id = 1;
char* event_data = "Hello World";
const uint32_t event_data_len = strlen(event_data) + 1;  // Plus 1 for NULL
bool enqueued = event_queue_put(&eq, event_id, event_data, event_data_len);
if (!enqueued) {
    // Queue full
}
```

## Get
```c
event_t* out_event = event_queue_get(&eq);
if (out_event != NULL) {
    // consume event
    event_queue_pop(&eq);  // Remove event from event queue
}
```


    
