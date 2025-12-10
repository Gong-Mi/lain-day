#include "time_utils.h"
#include "ecc_time.h"
#include "event_system.h"
#include <unistd.h>
#include <stdio.h>

// --- Constants for Epoch/Cycle counter ---
// The ECC scheme uses bits 0-29. We can use bits 30 and 31 for the epoch.
#define EPOCH_BIT_1 (1U << 30)
#define EPOCH_BIT_2 (1U << 31)
#define EPOCH_MASK (EPOCH_BIT_1 | EPOCH_BIT_2)
#define MAX_24_BIT_VALUE 0xFFFFFF

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

        uint32_t full_time_value = game_state->time_of_day;
        DecodedTimeResult decoded_result = decode_time_with_ecc(full_time_value);
        uint32_t current_data = decoded_result.data;

        uint32_t new_data = current_data + TIME_INCREMENT_PER_SECOND;

        // Check for 24-bit overflow to increment epoch
        if (new_data < current_data) {
            uint32_t current_epoch = (full_time_value & EPOCH_MASK);
            // Increment epoch (it's a 2-bit counter, 00 -> 01 -> 10 -> 11)
            if ((current_epoch & EPOCH_BIT_1) == 0) {
                current_epoch |= EPOCH_BIT_1;
            } else if ((current_epoch & EPOCH_BIT_2) == 0) {
                current_epoch &= ~EPOCH_BIT_1; // clear bit 1
                current_epoch |= EPOCH_BIT_2;  // set bit 2
            }
            // After 11, it stays at 11. 48 days is the max.
            full_time_value = (full_time_value & ~EPOCH_MASK) | current_epoch;
        }

        // Mask new_data to 24 bits just in case of overshoot
        new_data &= MAX_24_BIT_VALUE;

        uint32_t new_encoded_data = encode_time_with_ecc(new_data);
        
        // Preserve epoch bits and combine with new encoded data
        game_state->time_of_day = (full_time_value & EPOCH_MASK) | (new_encoded_data & ~EPOCH_MASK);
        
        pthread_mutex_unlock(&time_mutex);

        Event time_event;
        time_event.type = TIME_TICK_EVENT;
        push_event(time_event);
    }
    return NULL;
}

// --- Time Interpretation Functions ---

static int get_epoch(uint32_t time_of_day) {
    uint32_t epoch_bits = time_of_day & EPOCH_MASK;
    if (epoch_bits == (EPOCH_BIT_1 | EPOCH_BIT_2)) return 3;
    if (epoch_bits == EPOCH_BIT_2) return 2;
    if (epoch_bits == EPOCH_BIT_1) return 1;
    return 0;
}

int get_total_game_days(uint32_t time_of_day) {
    DecodedTimeResult decoded_result = decode_time_with_ecc(time_of_day);
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        return -1; // Indicate error
    }

    uint32_t raw_units = decoded_result.data;
    uint32_t total_seconds = raw_units / 16;
    uint32_t total_minutes = total_seconds / 60;
    uint32_t total_hours = total_minutes / 60;
    int days_in_cycle = total_hours / 24;

    int epoch = get_epoch(time_of_day);
    const int days_per_epoch = 12; // As calculated before, 2^24 raw units is ~12.13 days

    return (epoch * days_per_epoch) + days_in_cycle;
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
