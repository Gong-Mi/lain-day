#ifndef CHARACTER_MIKA_H
#define CHARACTER_MIKA_H

#include "game_types.h"
#include <stdbool.h>
#include <stdint.h>

// Forward declare GameState to avoid circular dependencies
struct GameState;

// Mika's Sanity Levels
typedef enum {
    MIKA_SANITY_NORMAL = 0,    // Normal daily routine
    MIKA_SANITY_IRRITATED = 1, // Routine mostly intact, dialogue changes
    MIKA_SANITY_PARANOID = 2,  // Breaks routine, hides in unexpected places
    MIKA_SANITY_BROKEN = 3     // Routine collapsed, catatonic or erratic
} MikaSanityLevel;

// A struct to encapsulate Mika's state and behaviors
typedef struct {
    // State
    const char* current_location_id;
    bool is_manually_positioned;
    MikaSanityLevel sanity_level;

    // Behaviors (function pointers)
    void (*on_talk)(struct GameState* game_state);
    bool (*is_room_accessible)(struct GameState* game_state, const struct Connection* connection);

} CharacterMika;

// Provides access to the single instance of Mika's logic module.
CharacterMika* get_mika_module(); // Changed to non-const to allow modification

// Defines a single entry in a character's daily schedule
typedef struct {
    uint32_t start_time_units; // Time of day (in 1/16s units) this entry begins
    const char* location_id;   // The location ID for this time block
} ScheduleEntry;

// Initializes the Mika module, setting up its function pointers.
void init_mika_module();

// Updates Mika's location based on the in-game time, schedule, and sanity.
// Returns the location ID where Mika should be.
const char* mika_update_location_by_schedule(struct GameState* game_state);

// Manually moves Mika to a specific location (for script-driven events).
void mika_move_to(const char* location_id);

// Resets Mika's positioning to be controlled by her schedule.
void mika_return_to_schedule(void);

// Restores Mika's state from a save file.
void restore_mika_state(const char* location_id, bool is_manual, int sanity_level);

// Sets Mika's sanity level (logic driver)
void mika_set_sanity(MikaSanityLevel level);

// Helper for debugging/testing: Calculates scheduled location without game state side effects
const char* mika_calculate_scheduled_location(uint32_t time_units_in_day, MikaSanityLevel sanity);

#endif // CHARACTER_MIKA_H