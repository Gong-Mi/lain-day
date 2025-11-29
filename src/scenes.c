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
#include "02a_talk_to_dad/scene.h"
#include "02d_talk_to_mom_normal/scene.h"
#include "01a_examine_navi/scene.h"
#include "01b_navi_shutdown/scene.h"
#include "01d_navi_reboot_endprologue/scene.h"
#include "01e_navi_connect_endprologue/scene.h"
#include "00a_wait_one_minute_endprologue/scene.h"
#include "01c_talk_to_figure_endprologue/scene.h"
#include "02g_mom_reply_silent_endprologue/scene.h"
#include "02f_mom_reply_fine_endprologue/scene.h"
#include "02b_dad_reply_no/scene.h"
#include "02c_dad_ask_help/scene.h"
#include "02j_get_milk_endprologue/scene.h"
#include "03_chapter_one_intro/scene.h"
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
    {"story/02a_talk_to_dad.md", init_scene_02a_talk_to_dad},
    {"story/02d_talk_to_mom_normal.md", init_scene_02d_talk_to_mom_normal},
    {"story/01a_examine_navi.md", init_scene_01a_examine_navi},
    {"story/01b_navi_shutdown.md", init_scene_01b_navi_shutdown},
    {"story/01d_navi_reboot_endprologue.md", init_scene_01d_navi_reboot_endprologue},
    {"story/01e_navi_connect_endprologue.md", init_scene_01e_navi_connect_endprologue},
    {"story/00a_wait_one_minute_endprologue.md", init_scene_00a_wait_one_minute_endprologue},
    {"story/01c_talk_to_figure_endprologue.md", init_scene_01c_talk_to_figure_endprologue},
    {"story/02g_mom_reply_silent_endprologue.md", init_scene_02g_mom_reply_silent_endprologue},
    {"story/02f_mom_reply_fine_endprologue.md", init_scene_02f_mom_reply_fine_endprologue},
    {"story/02b_dad_reply_no.md", init_scene_02b_dad_reply_no},
    {"story/02c_dad_ask_help.md", init_scene_02c_dad_ask_help},
    {"story/02j_get_milk_endprologue.md", init_scene_02j_get_milk_endprologue},
    {"story/03_chapter_one_intro.md", init_scene_03_chapter_one_intro},
    // NOTE: Add new scenes here as they are created.
};
static const int num_scene_registrations = sizeof(scene_registrations) / sizeof(SceneRegistration);


// --- Public API ---

#ifdef USE_DEBUG_LOGGING
int transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state) {
    fprintf(stderr, "DEBUG: Attempting to transition to scene: %s\n", target_story_file);
    fprintf(stderr, "DEBUG: transition_to_scene: scene ptr: %p\n", (void*)scene);
#else
int transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state) {
#endif
    // Clear the scene to ensure no leftover data
    memset(scene, 0, sizeof(StoryScene));

    // 1. Try to find the scene in our new dispatch table
    for (int i = 0; i < num_scene_registrations; ++i) {
        if (strcmp(target_story_file, scene_registrations[i].id) == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Scene '%s' found in dispatch table. Initializing...\n", target_story_file);
#endif
            scene_registrations[i].init_func(scene);
            
            // After successfully initializing, update the player's location if the scene specifies one.
            if (strlen(scene->location_id) > 0) {
                strncpy(game_state->player_state.location, scene->location_id, sizeof(game_state->player_state.location) - 1);
            }
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Successfully initialized scene '%s'.\n", target_story_file);
#endif
            return 1; // Success
        }
    }
    
    // 2. If not found, this is a scene we haven't converted yet.
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "ERROR: Scene '%s' is not registered in the C dispatch table.\n", target_story_file);
#endif
    return 0; // Failure: scene not found
}

