/*
 * Scene: 00_entry
 *
 * This file contains the hardcoded data for the initial entry scene.
 * The `init_scene_00_entry` function is responsible for populating the 
 * StoryScene struct with the scene's text, choices, and other metadata.
 * It is called by the scene transition manager in `src/scenes.c`.
 */

#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00_entry(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "00_entry", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "entry", MAX_NAME_LENGTH - 1);

    // Set scene text
    scene->text_content_ids[0] = TEXT_ENTRY_DESC1;
    scene->text_content_ids[1] = TEXT_ENTRY_DESC2;
    scene->text_content_ids[2] = TEXT_ENTRY_DESC3;
    scene->text_content_ids[3] = TEXT_ENTRY_DESC4;
    scene->text_content_ids[4] = TEXT_ENTRY_DESC5;
    scene->text_line_count = 5;

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_OPEN_DOOR;
    strncpy(scene->choices[0].action_id, "enter_lain_room", MAX_NAME_LENGTH - 1);

    scene->choices[1].text_id = TEXT_CHOICE_GO_DOWNSTAIRS;
    strncpy(scene->choices[1].action_id, "go_downstairs", MAX_NAME_LENGTH - 1);

    scene->choices[2].text_id = TEXT_CHOICE_WAIT_ONE_MINUTE;
    strncpy(scene->choices[2].action_id, "wait_one_minute", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 3;
}
