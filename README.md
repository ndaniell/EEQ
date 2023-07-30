# EEQ
Embedded Event Queue

This library provides a C implementation of a Event Queue. An Event is a message style structure, an identifier followed by accompanying data. 
To minimize the memory footprint it utilizes a circular buffer backend. The library utilizes padding to ensure contiguous memory for events and alignment.
Memory is provided during initialization. Utilizes atomics to achieve a thread safe solution using single producer single consumer. 

## Initialize
```c
uint8_t buffer[BUFFER_SIZE];
event_queue_t eq;
event_queue_config_t eq_config = { .buffer = buffer, 
                                   .buffer_len = BUFFER_SIZE, 
                                   .alignment = 4, 
                                   .use_atomics = true };
EventQueueInit(&eq, &eq_config);
```

## Put
```c
char* event_data = "Hello World";
const uint32_t event_data_len = strlen(event_data) + 1;  // Plus 1 for NULL
bool enqueued = EventQueuePut(&eq, 1, event_data, event_data_len);
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


    