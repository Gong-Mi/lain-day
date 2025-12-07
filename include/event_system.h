#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stdbool.h>

// Enum to define all possible event types in the game
typedef enum {
    EVENT_TYPE_NONE,
    TIME_TICK_EVENT,      // Fired every in-game second
    // Add other event types here in the future, e.g.,
    // INPUT_RECEIVED_EVENT,
    // SCENE_LOADED_EVENT,
} EventType;

// A generic Event structure. 
// It can be extended with a union to hold different data for different event types.
typedef struct {
    EventType type;
    // Example of event-specific data:
    // union {
    //     struct { int seconds_passed; } time_tick_data;
    //     struct { char* input; } input_received_data;
    // } data;
} Event;

#define MAX_EVENTS 100

// Initializes the event queue.
void init_event_queue(void);

// Pushes a new event to the queue.
// Returns true on success, false if the queue is full.
bool push_event(Event e);

// Polls for the next event from the queue.
// Returns true if an event was retrieved, false if the queue is empty.
bool poll_event(Event *e);

#endif // EVENT_SYSTEM_H
