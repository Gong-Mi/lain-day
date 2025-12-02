#ifndef CONDITIONS_H
#define CONDITIONS_H

#include <stdbool.h>

// Forward declare structs to avoid circular dependencies
struct GameState;
struct Connection;

// Condition function for Mika's room
bool is_mikas_room_accessible(struct GameState* game_state, const struct Connection* connection);

#endif // CONDITIONS_H
