#include "time_utils.h"
#include "ecc_time.h"
#include "event_system.h"
#include <unistd.h>
#include <stdio.h>

// Mutex for protecting game_state->time_of_day
pthread_mutex_t time_mutex;

// Flag to signal the time thread to stop
volatile bool game_is_running = true;

#define TIME_INCREMENT_PER_SECOND (1 * 16)

// Function run by the time thread to update game time
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

        // Decode the current 24-bit data from the 32-bit value
        DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);
        uint32_t current_data = decoded_result.data;

        // Increment the 24-bit data; overflow is handled by unsigned arithmetic
        uint32_t new_data = current_data + TIME_INCREMENT_PER_SECOND;

        // Re-encode the new 24-bit data
        uint32_t new_encoded_data = encode_time_with_ecc(new_data);

        // The upper 2 bits of game_state->time_of_day are random "noise" and are
        // intentionally preserved. We only update the lower 30 bits used by ECC.
        uint32_t preserved_noise = game_state->time_of_day & 0xC0000000; // Mask for top 2 bits
        game_state->time_of_day = preserved_noise | (new_encoded_data & 0x3FFFFFFF); // Combine noise and new 30-bit codeword
        
        pthread_mutex_unlock(&time_mutex);

        Event time_event;
        time_event.type = TIME_TICK_EVENT;
        push_event(time_event);
    }
    return NULL;
}

// --- Time Interpretation Functions ---

int get_total_game_days(uint32_t time_of_day) {
    DecodedTimeResult decoded_result = decode_time_with_ecc(time_of_day);
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        return -1; // Indicate error
    }

    // This calculation is based purely on the 24-bit data and will wrap around every ~12 days.
    uint32_t raw_units = decoded_result.data;
    uint32_t total_seconds = raw_units / 16;
    uint32_t total_minutes = total_seconds / 60;
    uint32_t total_hours = total_minutes / 60;
    
    return total_hours / 24;
}

int get_hour_of_day(uint32_t time_of_day) {
    DecodedTimeResult decoded_result = decode_time_with_ecc(time_of_day);
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        return -1; // Indicate error
    }

    uint32_t raw_units = decoded_result.data;
    uint32_t total_seconds = raw_units / 16;
    uint32_t total_minutes = total_seconds / 60;
    
    return (total_minutes / 60) % 24;
}
