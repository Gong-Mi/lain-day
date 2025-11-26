#ifndef SCENES_H
#define SCENES_H

// This file is now primarily a placeholder.
// Scene-specific headers are included directly by src/scenes.c.

#include "game_types.h"

// Function to transition to a new scene based on the story file path
int transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state);

#endif // SCENES_H

