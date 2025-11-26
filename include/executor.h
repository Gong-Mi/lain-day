#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "game_types.h"

// Executes an action based on its ID
// Returns 1 if the action caused a scene change, 0 otherwise.
int execute_action(const char* action_id, GameState* game_state);

// Executes a text-based command
void execute_command(const char* input, GameState* game_state);

#endif // EXECUTOR_H
