/*
 * scenes.c - Scene Transition Manager
 *
 * This file is responsible for mapping story file paths from action payloads
 * to their corresponding C-based scene initialization functions.
 * When a scene change is triggered, the `transition_to_scene` function
 * is called with the target story file path, and it populates the 
 * main StoryScene struct with the data for the new scene.
 */

#include "../include/scenes.h"
#include "../include/string_ids.h" // For StringID
#include "../include/game_types.h" // For StoryScene and GameState
#include <string.h>
#include <stdio.h> // For fprintf

// Include all individual scene headers from the new structure
#include "00_entry/scene.h"
#include "00a_wait_one_minute_endprologue/scene.h"
#include "01_lain_room/scene.h"
#include "01a_examine_navi/scene.h"
#include "01b_navi_shutdown/scene.h"
#include "01c_talk_to_figure_endprologue/scene.h"
#include "01d_navi_reboot_endprologue/scene.h"
#include "01e_navi_connect_endprologue/scene.h"
// TODO: Add more scene headers as they are converted

int transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state) {
    // Clear the scene to ensure no leftover data
    memset(scene, 0, sizeof(StoryScene));

    if (strcmp(target_story_file, "story/00_entry.md") == 0) {
        init_scene_00_entry(scene);
    } else if (strcmp(target_story_file, "story/00a_wait_one_minute_endprologue.md") == 0) {
        init_scene_00a_wait_one_minute_endprologue(scene);
    } else if (strcmp(target_story_file, "story/01_lain_room.md") == 0) {
        init_scene_01_lain_room(scene);
    } else if (strcmp(target_story_file, "story/01a_examine_navi.md") == 0) {
        init_scene_01a_examine_navi(scene);
    } else if (strcmp(target_story_file, "story/01b_navi_shutdown.md") == 0) {
        init_scene_01b_navi_shutdown(scene);
    } else if (strcmp(target_story_file, "story/01c_talk_to_figure_endprologue.md") == 0) {
        init_scene_01c_talk_to_figure_endprologue(scene);
    } else if (strcmp(target_story_file, "story/01d_navi_reboot_endprologue.md") == 0) {
        init_scene_01d_navi_reboot_endprologue(scene);
    } else if (strcmp(target_story_file, "story/01e_navi_connect_endprologue.md") == 0) {
        init_scene_01e_navi_connect_endprologue(scene);
    }
    // TODO: Add more scenes here as they are converted
    else {
        fprintf(stderr, "Error: Unknown target story file '%s' for scene transition.\n", target_story_file);
        return 0; // Failure
    }

    // After successfully initializing the scene, update the player's location
    // This assumes every scene sets its location_id correctly.
    strncpy(game_state->player_state.location, scene->location_id, MAX_NAME_LENGTH - 1);

    return 1; // Success
}

