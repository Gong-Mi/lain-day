/*
 * scenes.c - Scene Transition Manager
 *
 * This file is responsible for mapping story file paths from action payloads
 * to their corresponding C-based scene initialization functions.
 * It uses a dispatch table (`scene_registrations`) for modularity and scalability.
 */

#include "scenes.h" // Corrected include path
#include "game_types.h"
#include <string.h>
#include <stdio.h>

// --- Scene Headers ---
// Include all individual scene headers.
#include "00_entry/scene.h"
#include "01_lain_room/scene.h"
#include "02_downstairs/scene.h"
// NOTE: As more scenes are converted, their headers should be included here.
// For now, we are only including the prologue scenes we have created.
// To keep the game runnable, we will temporarily keep the old if-else for non-converted scenes.


// --- Dispatch Table ---
// A struct to map a scene ID (story file path) to its init function.
typedef struct {
    const char* id;
    void (*init_func)(StoryScene*);
} SceneRegistration;

// The central table for all registered C-based scenes.
static const SceneRegistration scene_registrations[] = {
    {"story/00_entry.md", init_scene_00_entry},
    {"story/01_lain_room.md", init_scene_01_lain_room},
    {"story/02_downstairs.md", init_scene_02_downstairs},
    // NOTE: Add new scenes here as they are created.
};
static const int num_scene_registrations = sizeof(scene_registrations) / sizeof(SceneRegistration);


// --- Public API ---

int transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state) {
    fprintf(stderr, "DEBUG: Attempting to transition to scene: %s\n", target_story_file);
    // Clear the scene to ensure no leftover data
    memset(scene, 0, sizeof(StoryScene));

    // 1. Try to find the scene in our new dispatch table
    for (int i = 0; i < num_scene_registrations; ++i) {
        if (strcmp(target_story_file, scene_registrations[i].id) == 0) {
            fprintf(stderr, "DEBUG: Scene '%s' found in dispatch table. Initializing...\n", target_story_file);
            scene_registrations[i].init_func(scene);
            
            // After successfully initializing, update the player's location if the scene specifies one.
            if (strlen(scene->location_id) > 0) {
                strncpy(game_state->player_state.location, scene->location_id, sizeof(game_state->player_state.location) - 1);
            }
            fprintf(stderr, "DEBUG: Successfully initialized scene '%s'.\n", target_story_file);
            return 1; // Success
        }
    }
    
    // 2. If not found, this is a scene we haven't converted yet.
    fprintf(stderr, "ERROR: Scene '%s' is not registered in the C dispatch table.\n", target_story_file);
    
    return 0; // Failure: scene not found
}

