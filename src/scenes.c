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
#include "02_downstairs/scene.h"
#include "03_chapter_one_intro/scene.h"
#include "02d_talk_to_mom_normal/scene.h"
#include "02a_talk_to_dad/scene.h"
#include "02b_dad_reply_no/scene.h"
#include "02c_dad_ask_help/scene.h"
#include "02j_get_milk_endprologue/scene.h"
#include "04a_talk_to_sister_cold/scene.h"
#include "04b_talk_to_sister_curious/scene.h"
#include "04c_talk_to_sister_default/scene.h"
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
    } else if (strcmp(target_story_file, "story/02_downstairs.md") == 0) {
        init_scene_02_downstairs(scene);
    } else if (strcmp(target_story_file, "story/03_chapter_one_intro.md") == 0) {
        init_scene_03_chapter_one_intro(scene);
    } else if (strcmp(target_story_file, "story/02d_talk_to_mom_normal.md") == 0) {
        init_scene_02d_talk_to_mom_normal(scene);
    } else if (strcmp(target_story_file, "story/02a_talk_to_dad.md") == 0) {
        init_scene_02a_talk_to_dad(scene);
    } else if (strcmp(target_story_file, "story/02b_dad_reply_no.md") == 0) {
        init_scene_02b_dad_reply_no(scene);
    } else if (strcmp(target_story_file, "story/02c_dad_ask_help.md") == 0) {
        init_scene_02c_dad_ask_help(scene);
    } else if (strcmp(target_story_file, "story/02j_get_milk_endprologue.md") == 0) {
        init_scene_02j_get_milk_endprologue(scene);
    } else if (strcmp(target_story_file, "story/04a_talk_to_sister_cold.md") == 0) {
        init_scene_04a_talk_to_sister_cold(scene);
    } else if (strcmp(target_story_file, "story/04b_talk_to_sister_curious.md") == 0) {
        init_scene_04b_talk_to_sister_curious(scene);
    } else if (strcmp(target_story_file, "story/04c_talk_to_sister_default.md") == 0) {
        init_scene_04c_talk_to_sister_default(scene);
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

