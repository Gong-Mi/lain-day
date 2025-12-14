#include "characters/mika.h"
#include "game_types.h"
#include "flag_system.h"
#include "executor.h"
#include "ecc_time.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// --- Mika's Daily Schedule ---

static const ScheduleEntry MIKA_SCHEDULE[] = {
    // Times are in 1/16s units from the start of the day (00:00)
    { (0 * 60 * 60 * 16), "iwakura_mikas_room" },           // 00:00 - 07:59 (Sleeping)
    { (8 * 60 * 60 * 16), "iwakura_living_dining_kitchen" },// 08:00 - 08:59 (Breakfast)
    { (9 * 60 * 60 * 16), "off_map" },                      // 09:00 - 16:59 (At school/away)
    { (17 * 60 * 60 * 16), "iwakura_mikas_room" },          // 17:00 - 19:59 (In her room)
    { (20 * 60 * 60 * 16), "iwakura_living_dining_kitchen" },// 20:00 - 21:59 (In living room)
    { (22 * 60 * 60 * 16), "iwakura_mikas_room" },          // 22:00 onwards (In her room)
};
static const int MIKA_SCHEDULE_ENTRIES = sizeof(MIKA_SCHEDULE) / sizeof(ScheduleEntry);


// --- Private Functions (Logic moved from other parts of the engine) ---

// Logic moved from src/conditions.c
static bool mika_is_room_accessible_impl(struct GameState* game_state, const struct Connection* connection) {
    if (game_state == NULL) {
        return false;
    }

    uint32_t encoded_time;

    // Accessing time safely is complex here without direct access to the mutex.
    // For now, we assume this function is called from a context where time is stable.
    // A better implementation would involve passing the mutex or a timestamp.
    encoded_time = game_state->time_of_day;
    
    DecodedTimeResult decoded_result = decode_time_with_ecc(encoded_time);

    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        return false; // Door is definitely locked if time is glitching
    }

    uint32_t current_time_units = decoded_result.data;
    const uint32_t start_time_units = 17 * 60 * 60 * 16; // 17:00 (Using literal value for clarity)
    const uint32_t end_time_units = 21 * 60 * 60 * 16;   // 21:00
    const uint32_t units_in_a_day = 24 * 60 * 60 * 16;
    
    uint32_t time_units_in_day = current_time_units % units_in_a_day;

    // Door is open between 17:00 and 21:00
    return (time_units_in_day >= start_time_units && time_units_in_day < end_time_units);
}

// Logic moved from src/executor.c
static void mika_on_talk_impl(struct GameState* game_state) {
    const char* sister_mood = hash_table_get(game_state->flags, "sister_mood");
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: mika_on_talk_impl: current sister_mood: %s\n", sister_mood ? sister_mood : "NULL");
#endif
    if (sister_mood != NULL) {
        if (strcmp(sister_mood, "cold") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: mika_on_talk_impl: Executing 'talk_to_sister_cold'\n");
#endif
            execute_action("talk_to_sister_cold", game_state);
        } else if (strcmp(sister_mood, "curious") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: mika_on_talk_impl: Executing 'talk_to_sister_curious'\n");
#endif
            execute_action("talk_to_sister_curious", game_state);
        } else {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: mika_on_talk_impl: Executing 'talk_to_sister_default'\n");
#endif
            execute_action("talk_to_sister_default", game_state);
        }
    } else {
#ifdef USE_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: mika_on_talk_impl: sister_mood is NULL. Executing 'talk_to_sister_default'\n");
#endif
        execute_action("talk_to_sister_default", game_state);
    }
    // After the conversation is initiated, Mika moves. This is a script-driven move.
    // Mika's movement should be handled by schedule or explicit plot events.
}


// --- Module Definition ---

// The single instance of the Mika module's data and function pointers.
static CharacterMika g_mika_module;

// --- Public API Implementation ---

void init_mika_module() {
    g_mika_module.on_talk = mika_on_talk_impl;
    g_mika_module.is_room_accessible = mika_is_room_accessible_impl;
    // Initial location is null; it will be set by the first schedule update.
    g_mika_module.current_location_id = "UNKNOWN_LOCATION"; 
    g_mika_module.is_manually_positioned = false;
}

const CharacterMika* get_mika_module() {
    return &g_mika_module;
}

void mika_move_to(const char* location_id) {
    if (location_id) {
        g_mika_module.current_location_id = location_id;
        g_mika_module.is_manually_positioned = true; // Mark that a script is controlling her position
#ifdef USE_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Mika script-moved to location: %s\n", location_id);
#endif
    }
}

void mika_update_location_by_schedule(struct GameState* game_state) {
    if (!game_state) return;

    // If Mika's position is being controlled by a script, don't update from schedule.
    if (g_mika_module.is_manually_positioned) {
        return;
    }

    // Decode time to get absolute time units
    DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        // If time is glitching, maybe Mika disappears or stays put.
        // For now, we'll have her disappear from the map.
        g_mika_module.current_location_id = "off_map";
        return;
    }

    // Calculate the time of day in our 1/16s units
    const uint32_t units_in_a_day = 24 * 60 * 60 * 16;
    uint32_t time_units_in_day = decoded_result.data % units_in_a_day;

    // Find the correct schedule entry by iterating backwards
    const char* new_location_id = NULL;
    for (int i = MIKA_SCHEDULE_ENTRIES - 1; i >= 0; --i) {
        if (time_units_in_day >= MIKA_SCHEDULE[i].start_time_units) {
            new_location_id = MIKA_SCHEDULE[i].location_id;
            break;
        }
    }

    // Update location only if it has changed
    if (new_location_id && (!g_mika_module.current_location_id || strcmp(g_mika_module.current_location_id, new_location_id) != 0)) {
        g_mika_module.current_location_id = new_location_id;
#ifdef USE_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Mika location updated by schedule to: %s\n", new_location_id);
#endif
    }
}

void mika_return_to_schedule(void) {
    g_mika_module.is_manually_positioned = false;
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Mika has been returned to schedule-based positioning.\n");
#endif
}
