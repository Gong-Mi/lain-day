#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <stdbool.h>
#include "game_types.h" // For GameState and Condition structs

// Function to check if a set of conditions are met
bool check_conditions(const struct GameState* game_state, const Condition* conditions, int condition_count);

#endif // CONDITIONS_H
