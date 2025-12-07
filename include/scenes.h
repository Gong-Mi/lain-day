#ifndef SCENES_H
#define SCENES_H

// This file is now primarily a placeholder.
// Scene-specific headers are included directly by src/scenes.c.

#include "game_types.h"

// Transitions the game to the specified scene by its ID.
// Returns true on success, false if the scene ID is not found.
bool transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state);

// Checks if a choice is currently selectable based on its conditions and the game state.
bool is_choice_selectable(const StoryChoice* choice, const GameState* game_state);

#endif // SCENES_H

