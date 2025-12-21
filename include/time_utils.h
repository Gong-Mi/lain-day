#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <pthread.h>
#include <stdbool.h>
#include "game_types.h" // For GameState

// Mutex for protecting game_state->time_of_day
extern pthread_mutex_t time_mutex;

// Flag to signal the time thread to stop
extern volatile bool game_is_running;

// Thread function
void* time_thread_func(void* arg);

// Real-time functions
uint64_t get_current_time_ms();

// Time interpretation functions
int get_total_game_days(uint32_t time_of_day);
int get_hour_of_day(uint32_t time_of_day);
int get_minute_of_hour(uint32_t time_of_day);
int get_second_of_minute(uint32_t time_of_day);

#endif // TIME_UTILS_H
