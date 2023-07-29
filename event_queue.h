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
        circular_buffer_t _cb;
    } event_queue_t;

    /**
     * Initialize the event queue
     *
     * @param eq Event Queue
    */
    static inline bool EventQueueInit(event_queue_t* eq, void* buffer, uint32_t buffer_size)
    {
        CircularBufferInit(&eq->_cb, buffer, buffer_size);
        return true;
    }

    /**
     * Clear the event queue
     *
     * @param eq Event Queue
    */
    static inline void EventQueueClear(event_queue_t* eq)
    {
        CircularBufferClear(&eq->_cb);
    }

    /**
     * Put an event off the event queue
     *
     * @param eq Event Queue
     * @param event Poiner to the event to place in the queue (done by copy)
    */
    static inline bool EventQueuePut(event_queue_t* eq, event_t* event)
    {
        uint32_t event_size = sizeof(event_t);
        if (event->event_data != NULL && event->event_data_length > 0) {
            event_size += event->event_data_length;
        }
        return CircularBufferProduceBytes(&eq->_cb, (void*)event, event_size);
    }

    /**
     * Get an event off the event queue
     *
     * @param eq Event Queue
     * @return Pointer to the event - NULL if no event
    */
    static inline event_t* EventQueueGet(event_queue_t* eq)
    {
        uint32_t available_bytes;
        event_t* evt = (event_t*)CircularBufferTail(&eq->_cb, &available_bytes);
        if (available_bytes < sizeof(event_t)) {
            evt = NULL;
        }
        return evt;
    }

    /**
     * Remove an event from the event queue
     *
     * @param eq Event Queue
    */
    static inline void EventQueuePop(event_queue_t* eq)
    {
        event_t* evt = EventQueueGet(eq);
        if (evt != NULL) {
            CircularBufferConsume(&eq->_cb, sizeof(event_t) + evt->event_data_length);
        }
    }

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // EVENT_QUEUE_H
