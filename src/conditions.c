#include "conditions.h"
#include "game_types.h"
#include "time_utils.h"
#include "flag_system.h"
#include <string.h>
#include <stdio.h>

bool check_conditions(const struct GameState* game_state, const Condition* conditions, int condition_count) {
    if (condition_count == 0) {
        return true;
    }

    int current_day = -1;
    int current_hour = -1;
    bool time_fetched = false;

    for (int i = 0; i < condition_count; i++) {
        const Condition* cond = &conditions[i];
        bool time_condition_exists = cond->min_day != -1 || cond->max_day != -1 || cond->exact_day != -1 || cond->hour_start != -1;

        // --- Check time requirements ---
        if (time_condition_exists) {
            if (!time_fetched) {
                current_day = get_total_game_days(game_state->time_of_day);
                current_hour = get_hour_of_day(game_state->time_of_day);
                time_fetched = true;
            }

            if (cond->exact_day != -1 && current_day != cond->exact_day) return false;
            if (cond->min_day != -1 && current_day < cond->min_day) return false;
            if (cond->max_day != -1 && current_day > cond->max_day) return false;
            if (cond->hour_start != -1 && current_hour < cond->hour_start) return false;
            if (cond->hour_end != -1 && current_hour > cond->hour_end) return false;
        }

        // --- Check flag requirement ---
        if (cond->flag_name[0] != '\0') {
            const char* current_flag_value = hash_table_get(game_state->flags, cond->flag_name);
            if (cond->required_value[0] != '\0') {
                // We need the flag to have a specific value
                if (current_flag_value == NULL || strcmp(current_flag_value, cond->required_value) != 0) {
                    return false;
                }
            } else {
                // We just need the flag to be set to "1" (or any non-null, non-"0" value)
                if (current_flag_value == NULL || strcmp(current_flag_value, "0") == 0) {
                    return false;
                }
            }
        }
    }

    return true; // All conditions met
}

