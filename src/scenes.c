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
#include "02_mom_arc/scene.h"
#include "01a_examine_navi/scene.h"
#include "01b_navi_shutdown/scene.h"
#include "01d_navi_reboot_endprologue/scene.h"
#include "01e_navi_connect_endprologue/scene.h"
#include "00a_wait_one_minute_endprologue/scene.h"
#include "01c_talk_to_figure_endprologue/scene.h"

#include "02b_dad_reply_no/scene.h"
#include "02c_dad_ask_help/scene.h"
#include "02j_get_milk_endprologue/scene.h"
#include "03_chapter_one_intro/scene.h"
#include "04c_talk_to_sister_default/scene.h"
#include "04a_talk_to_sister_cold/scene.h"
#include "04b_talk_to_sister_curious/scene.h"
#include "mika_room_locked/scene.h"
#include "mika_room_unlocked/scene.h"
#include "iwakura_upper_hallway/scene.h"
#include "examine_fridge/scene.h"
#include "SCENE_00_ENTRY_data.h" // Include the generated header for 00_entry scene
// NOTE: As more scenes are converted, their headers should be included here.
// For now, we are only including the prologue scenes we have created.
// To keep the game runnable, we will temporarily keep the old if-else for non-converted scenes.


// --- Scene Initializers ---

// New Time Glitch Scene
void init_scene_time_glitch(StoryScene* scene) {
    strcpy(scene->scene_id, "SCENE_TIME_GLITCH");
    strcpy(scene->name, "Time Glitch");
    strcpy(scene->location_id, "the_wired");

    scene->dialogue_line_count = 4;
    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, SID_TIME_GLITCH_1};
    scene->dialogue_lines[1] = (DialogueLine){SPEAKER_NONE, SID_TIME_GLITCH_2};
    scene->dialogue_lines[2] = (DialogueLine){SPEAKER_NONE, SID_TIME_GLITCH_3};
    scene->dialogue_lines[3] = (DialogueLine){SPEAKER_NONE, SID_TIME_GLITCH_4};

    scene->choice_count = 1;
    scene->choices[0] = (StoryChoice){SID_TIME_GLITCH_CHOICE_1, "reset_from_glitch", {}};
}


// --- Dispatch Table ---
// A struct to map a scene ID (story file path) to its init function.
typedef struct {
    const char* id;
    void (*init_func)(StoryScene*);
} SceneRegistration;

// The central table for all registered C-based scenes.
static const SceneRegistration scene_registrations[] = {
    {"SCENE_00_ENTRY", init_scene_scene_00_entry_from_data},
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
    {"story/04c_talk_to_sister_default.md", init_scene_04c_talk_to_sister_default},
    {"story/04a_talk_to_sister_cold.md", init_scene_04a_talk_to_sister_cold},
    {"story/04b_talk_to_sister_curious.md", init_scene_04b_talk_to_sister_curious},
    {"SCENE_MIKA_ROOM_LOCKED", init_scene_mika_room_locked},
    {"SCENE_MIKA_ROOM_UNLOCKED", init_scene_mika_room_unlocked},
    {"SCENE_IWAKURA_UPPER_HALLWAY", init_scene_iwakura_upper_hallway},
    {"SCENE_TIME_GLITCH", init_scene_time_glitch},
    {"SCENE_EXAMINE_FRIDGE", init_scene_examine_fridge},
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

