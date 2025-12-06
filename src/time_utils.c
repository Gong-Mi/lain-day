#include "time_utils.h"
#include "ecc_time.h"
#include "characters/mika.h" // For schedule updates
#include <unistd.h> // For sleep

// Mutex for protecting game_state->time_of_day
pthread_mutex_t time_mutex;

// Flag to signal the time thread to stop
volatile bool game_is_running = true;

// Total time units in a day (24 hours * 60 minutes * 60 seconds * 16 units/sec)
#define UNITS_IN_A_DAY (24 * 60 * 60 * 16)
// Time increment per second (1 minute * 60 seconds * 16 units/sec)
// To keep the old rate of 1 minute passing per real-time second.
// We will actually increment by 1 second of game time per real time second for now.
#define TIME_INCREMENT_PER_SECOND (1 * 16)


// Function run by the time thread to update game time
void* time_thread_func(void* arg) {
    GameState* game_state = (GameState*)arg;
    static int tick_count = 0;
    while (game_is_running) {
        usleep(100000); // Sleep for 100ms
        
        tick_count++;
        if (tick_count < 10) {
            continue; // Skip time update until 1 second has passed
        }
        tick_count = 0; // Reset counter

        pthread_mutex_lock(&time_mutex);

        // Decode the current time
        DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);

        // For now, we ignore errors and just advance time.
        // Later, DOUBLE_BIT_ERROR_DETECTED will trigger a 'time glitch'.
        uint32_t current_time = decoded_result.data;

        // Increment time (monotonically increasing)
        uint32_t new_time = current_time + TIME_INCREMENT_PER_SECOND;

        // Re-encode and update
        game_state->time_of_day = encode_time_with_ecc(new_time);

        // Update character locations based on the new time
        mika_update_location_by_schedule(game_state);

        pthread_mutex_unlock(&time_mutex);
    }
    return NULL;
}
