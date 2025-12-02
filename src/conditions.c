#include "conditions.h"
#include "game_types.h" // For the full GameState definition
#include "time_utils.h" // For time_mutex and game_is_running
#include <pthread.h> // Though time_utils.h includes it, keep for clarity.

extern pthread_mutex_t time_mutex; // This is now defined in time_utils.c, declared in time_utils.h

bool is_mikas_room_accessible(struct GameState* game_state, const struct Connection* connection) {
    if (game_state == NULL) {
        return false;
    }

    int current_time;

    // Lock the mutex to safely read the time
    pthread_mutex_lock(&time_mutex);
    current_time = game_state->time_of_day;
    pthread_mutex_unlock(&time_mutex);
    
    // Door is open between 17:00 (1020) and 21:00 (1260)
    if (current_time >= 1020 && current_time <= 1260) {
        return true;
    }
    
    return false;
}
