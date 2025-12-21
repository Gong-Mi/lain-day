#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "game_types.h"

// Executes an action based on its ID
// Returns 1 if the action caused a scene change, 0 otherwise.
int execute_action(const char* action_id, GameState* game_state);

// Executes a text-based command
bool execute_command(const char* input, GameState* game_state);

// Checks for auto-triggered events in the current scene
// Returns true if an event was triggered and the scene changed
bool check_and_trigger_auto_events(GameState* game_state, StoryScene* current_scene, uint32_t scene_entry_time);

#endif // EXECUTOR_H
