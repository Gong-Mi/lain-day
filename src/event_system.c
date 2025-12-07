#include "event_system.h"
#include <pthread.h>

// A simple circular queue for events
static Event event_queue[MAX_EVENTS];
static int queue_head = 0;
static int queue_tail = 0;
static int event_count = 0;

// Mutex to make the event queue thread-safe
static pthread_mutex_t queue_mutex;

void init_event_queue(void) {
    pthread_mutex_init(&queue_mutex, NULL);
    queue_head = 0;
    queue_tail = 0;
    event_count = 0;
}

bool push_event(Event e) {
    pthread_mutex_lock(&queue_mutex);
    
    if (event_count >= MAX_EVENTS) {
        // Queue is full
        pthread_mutex_unlock(&queue_mutex);
        return false;
    }
    
    event_queue[queue_tail] = e;
    queue_tail = (queue_tail + 1) % MAX_EVENTS;
    event_count++;
    
    pthread_mutex_unlock(&queue_mutex);
    return true;
}

bool poll_event(Event *e) {
    pthread_mutex_lock(&queue_mutex);
    
    if (event_count == 0) {
        // Queue is empty
        pthread_mutex_unlock(&queue_mutex);
        return false;
    }
    
    *e = event_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_EVENTS;
    event_count--;
    
    pthread_mutex_unlock(&queue_mutex);
    return true;
}
