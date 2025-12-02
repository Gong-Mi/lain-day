#include "time_utils.h"
#include <unistd.h> // For sleep

// Mutex for protecting game_state->time_of_day
pthread_mutex_t time_mutex;

// Flag to signal the time thread to stop
volatile bool game_is_running = true;

// Function run by the time thread to update game time
void* time_thread_func(void* arg) {
    GameState* game_state = (GameState*)arg; // Note: GameState is a typedef for struct GameState
    while (game_is_running) {
        sleep(1);
        pthread_mutex_lock(&time_mutex);
        game_state->time_of_day = (game_state->time_of_day + 1) % (24 * 60);
        pthread_mutex_unlock(&time_mutex);
    }
    return NULL;
}
