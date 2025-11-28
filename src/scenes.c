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
    // Clear the scene to ensure no leftover data
    memset(scene, 0, sizeof(StoryScene));

    // 1. Try to find the scene in our new dispatch table
    for (int i = 0; i < num_scene_registrations; ++i) {
        if (strcmp(target_story_file, scene_registrations[i].id) == 0) {
            scene_registrations[i].init_func(scene);
            
            // After successfully initializing, update the player's location if the scene specifies one.
            if (strlen(scene->location_id) > 0) {
                strncpy(game_state->player_state.location, scene->location_id, sizeof(game_state->player_state.location) - 1);
            }
            return 1; // Success
        }
    }
    
    // 2. If not found, this is a scene we haven't converted yet.
    // For now, we can print a warning. In a full implementation, this might
    // fall back to a markdown parser or simply error out.
    fprintf(stderr, "Warning: Scene '%s' is not registered in the C dispatch table. "
                    "This scene has not been converted from Markdown yet.\n", target_story_file);


    // Here you could add a fallback to the old if-else chain or a markdown parser
    // For this task, we will consider it a failure to enforce the C-conversion process.
    
    return 0; // Failure: scene not found
}

