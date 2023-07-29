# EEQ
Embedded Event Queue

This library provides a C implementation of a Event Queue. An Event is a message style structure, an identifier followed by accomponing data. 
To minimize the memory footprint it utilizes a circular buffer backend. Memory is provided during initialization. Utilizes atomics to achieve 
a thread safe solution using single producer single consumer.


## Initialize
```c
uint8_t buffer[BUFFER_SIZE];
event_queue_t eq;
EventQueueInit(&eq, buffer, BUFFER_SIZE);
```

## Put
```c
char* event_data = "Hello World";
event_t event = { .event_id = 1, 
                  .event_data_length = strlen(event_data) + 1, 
                  .event_data = event_data };
bool enqueued = EventQueuePut(&eq, &event);
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


    