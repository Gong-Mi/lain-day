#include "time_utils.h"
#include "ecc_time.h"
#include "event_system.h" // Include the new event system
#include <unistd.h> // For usleep

// Mutex for protecting game_state->time_of_day
pthread_mutex_t time_mutex;

// Flag to signal the time thread to stop
volatile bool game_is_running = true;

#define TIME_INCREMENT_PER_SECOND (1 * 16)

// Function run by the time thread to update game time and push events
void* time_thread_func(void* arg) {
    GameState* game_state = (GameState*)arg;
    static int tick_count = 0;
    while (game_is_running) {
        usleep(100000); // Sleep for 100ms
        
        tick_count++;
        if (tick_count < 10) {
            continue; // Skip until 1 second has passed
        }
        tick_count = 0; // Reset counter

        pthread_mutex_lock(&time_mutex);

        // Decode the current time
        DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);
        uint32_t current_time = decoded_result.data;

        // Increment time
        uint32_t new_time = current_time + TIME_INCREMENT_PER_SECOND;

        // Re-encode and update
        game_state->time_of_day = encode_time_with_ecc(new_time);
        
        pthread_mutex_unlock(&time_mutex);

        // Push a time tick event to the queue
        Event time_event;
        time_event.type = TIME_TICK_EVENT;
        push_event(time_event);
    }
    return NULL;
}
