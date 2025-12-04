#ifndef CHARACTER_MIKA_H
#define CHARACTER_MIKA_H

#include "game_types.h"
#include <stdbool.h>

// Forward declare GameState to avoid circular dependencies if Mika needs more complex state
struct GameState;

// A struct to encapsulate Mika's state and behaviors
typedef struct {
    // State
    const char* current_location_id;

    // Behaviors (function pointers)
    void (*on_talk)(struct GameState* game_state);
    bool (*is_room_accessible)(struct GameState* game_state, const struct Connection* connection);

} CharacterMika;

// Provides access to the single instance of Mika's logic module.
const CharacterMika* get_mika_module();


// Defines a single entry in a character's daily schedule
typedef struct {
    uint32_t start_time_units; // Time of day (in 1/16s units) this entry begins
    const char* location_id;   // The location ID for this time block
} ScheduleEntry;

// Initializes the Mika module, setting up its function pointers.
void init_mika_module();

// Updates Mika's location based on the in-game time and her schedule.
void mika_update_location_by_schedule(struct GameState* game_state);

// Manually moves Mika to a specific location (for script-driven events).
void mika_move_to(const char* location_id);

#endif // CHARACTER_MIKA_H
