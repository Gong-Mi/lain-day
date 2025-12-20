#include "characters/mika.h"
#include "game_types.h"
#include "flag_system.h"
#include "executor.h"
#include "ecc_time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // for rand()

// --- Mika's Daily Schedule Definitions ---

// Normal Routine (Sanity 0 & 1)
static const ScheduleEntry MIKA_SCHEDULE_NORMAL[] = {
    { (0 * 60 * 60 * 16), "iwakura_mikas_room" },           // 00:00 - 07:59 (Sleeping)
    { (7 * 60 * 60 * 16), "iwakura_bathroom" },             // 07:00 - 07:59 (Getting ready) - NEW
    { (8 * 60 * 60 * 16), "iwakura_living_dining_kitchen" },// 08:00 - 08:59 (Breakfast)
    { (9 * 60 * 60 * 16), "off_map" },                      // 09:00 - 16:59 (School/Away)
    { (17 * 60 * 60 * 16), "iwakura_mikas_room" },          // 17:00 - 19:59 (Room)
    { (20 * 60 * 60 * 16), "iwakura_living_dining_kitchen" },// 20:00 - 21:59 (Dinner/TV)
    { (22 * 60 * 60 * 16), "iwakura_mikas_room" },          // 22:00 onwards (Sleep)
};
static const int MIKA_SCHEDULE_NORMAL_COUNT = sizeof(MIKA_SCHEDULE_NORMAL) / sizeof(ScheduleEntry);

// Paranoid Routine (Sanity 2) - Breaking patterns
static const ScheduleEntry MIKA_SCHEDULE_PARANOID[] = {
    { (0 * 60 * 60 * 16), "iwakura_mikas_room" },           // Night: Hiding in room
    { (10 * 60 * 60 * 16), "iwakura_bathroom" },            // 10:00: Hiding in bathroom (should be at school)
    { (12 * 60 * 60 * 16), "shibuya_street" },              // 12:00: Wandering in Shibuya
    { (16 * 60 * 60 * 16), "iwakura_lower_hallway" },       // 16:00: Staring at the phone in hallway
    { (18 * 60 * 60 * 16), "iwakura_mikas_room" },          // 18:00: Back in room, locked
};
static const int MIKA_SCHEDULE_PARANOID_COUNT = sizeof(MIKA_SCHEDULE_PARANOID) / sizeof(ScheduleEntry);


// --- Private Functions ---

static bool mika_is_room_accessible_impl(struct GameState* game_state, const struct Connection* connection) {
    if (!game_state) return false;

    // Key override
    for (int i = 0; i < game_state->player_state.inventory_count; i++) {
        if (strcmp(game_state->player_state.inventory[i].name, "key_mika_room") == 0) {
            return true;
        }
    }

    // Time-based access
    uint32_t encoded_time = game_state->time_of_day;
    DecodedTimeResult decoded_result = decode_time_with_ecc(encoded_time);

    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) return false;

    uint32_t current_time_units = decoded_result.data;
    const uint32_t units_in_a_day = 24 * 60 * 60 * 16;
    uint32_t time_units_in_day = current_time_units % units_in_a_day;

    // Normal State: Open 17:00 - 21:00
    // Broken State: Always locked
    CharacterMika* mika = get_mika_module();
    if (mika->sanity_level >= MIKA_SANITY_BROKEN) {
        return false; 
    }

    // Convert to hours for easier logic
    int hour = (time_units_in_day / 16) / 3600;
    return (hour >= 17 && hour < 21);
}

static void mika_on_talk_impl(struct GameState* game_state) {
    CharacterMika* mika = get_mika_module();
    
    // Dispatch based on Sanity Level first
    if (mika->sanity_level == MIKA_SANITY_BROKEN) {
        printf("Mika stares at the wall, mumbling something about the prophecy.\n");
        return;
    }
    
    // Standard mood-based dispatch
    const char* sister_mood = hash_table_get(game_state->flags, "sister_mood");
    if (sister_mood) {
        if (strcmp(sister_mood, "cold") == 0) execute_action("talk_to_sister_cold", game_state);
        else if (strcmp(sister_mood, "curious") == 0) execute_action("talk_to_sister_curious", game_state);
        else execute_action("talk_to_sister_default", game_state);
    } else {
        execute_action("talk_to_sister_default", game_state);
    }
}

// --- Module Instance ---

static CharacterMika g_mika_module;

// --- Public API ---

void init_mika_module() {
    g_mika_module.on_talk = mika_on_talk_impl;
    g_mika_module.is_room_accessible = mika_is_room_accessible_impl;
    g_mika_module.current_location_id = "UNKNOWN_LOCATION"; 
    g_mika_module.is_manually_positioned = false;
    g_mika_module.sanity_level = MIKA_SANITY_NORMAL;
}

CharacterMika* get_mika_module() {
    return &g_mika_module;
}

void mika_set_sanity(MikaSanityLevel level) {
    g_mika_module.sanity_level = level;
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Mika Sanity set to %d\n", level);
#endif
}

void mika_move_to(const char* location_id) {
    if (location_id) {
        g_mika_module.current_location_id = location_id;
        g_mika_module.is_manually_positioned = true;
    }
}

// Pure logic function: Returns scheduled location based on time and sanity
const char* mika_calculate_scheduled_location(uint32_t time_units_in_day, MikaSanityLevel sanity) {
    const char* new_location_id = "off_map"; // Default

    if (sanity >= MIKA_SANITY_BROKEN) {
        return "iwakura_mikas_room"; // Always in room when broken
    }

    const ScheduleEntry* schedule;
    int count;

    if (sanity == MIKA_SANITY_PARANOID) {
        schedule = MIKA_SCHEDULE_PARANOID;
        count = MIKA_SCHEDULE_PARANOID_COUNT;
    } else {
        // Normal & Irritated share the base schedule
        schedule = MIKA_SCHEDULE_NORMAL;
        count = MIKA_SCHEDULE_NORMAL_COUNT;
    }

    // Iterate backwards to find the active slot
    for (int i = count - 1; i >= 0; --i) {
        if (time_units_in_day >= schedule[i].start_time_units) {
            new_location_id = schedule[i].location_id;
            break;
        }
    }
    
    return new_location_id;
}

const char* mika_update_location_by_schedule(struct GameState* game_state) {
    if (!game_state) return NULL;
    if (g_mika_module.is_manually_positioned) return g_mika_module.current_location_id;

    DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        g_mika_module.current_location_id = "off_map";
        return "off_map";
    }

    const uint32_t units_in_a_day = 24 * 60 * 60 * 16;
    uint32_t time_units_in_day = decoded_result.data % units_in_a_day;

    const char* new_loc = mika_calculate_scheduled_location(time_units_in_day, g_mika_module.sanity_level);
    
    // Update module state
    if (new_loc && (!g_mika_module.current_location_id || strcmp(g_mika_module.current_location_id, new_loc) != 0)) {
        g_mika_module.current_location_id = new_loc;
#ifdef USE_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Mika moved to %s (Sanity: %d)\n", new_loc, g_mika_module.sanity_level);
#endif
    }
    return g_mika_module.current_location_id;
}

void mika_return_to_schedule(void) {
    g_mika_module.is_manually_positioned = false;
}

void restore_mika_state(const char* location_id, bool is_manual, int sanity_level) {
    if (location_id) g_mika_module.current_location_id = location_id;
    g_mika_module.is_manually_positioned = is_manual;
    g_mika_module.sanity_level = (MikaSanityLevel)sanity_level;
}