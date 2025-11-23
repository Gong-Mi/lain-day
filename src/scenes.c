#include "../include/scenes.h"
#include "../include/string_ids.h"
#include <string.h>

void init_scene_examine_navi(StoryScene* scene) {
    // Clear the scene to ensure no leftover data
    memset(scene, 0, sizeof(StoryScene));

    // Set Scene ID and Location
    strncpy(scene->scene_id, "navi_examine", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "lain_room", MAX_NAME_LENGTH - 1); // From 01_lain_room.md context

    // Set Text Content
    scene->text_content_ids[scene->text_line_count++] = TEXT_NAVI_STATE_TITLE;
    scene->text_content_ids[scene->text_line_count++] = TEXT_NAVI_STATE_DESC1;
    scene->text_content_ids[scene->text_line_count++] = TEXT_NAVI_STATE_DESC2;
    scene->text_content_ids[scene->text_line_count++] = TEXT_NAVI_STATE_DESC3;
    scene->text_content_ids[scene->text_line_count++] = TEXT_NAVI_STATE_DESC4;

    // Set Choices
    // Choice 1: navi_shutdown
    scene->choices[scene->choice_count].text_id = TEXT_CHOICE_NAVI_SHUTDOWN;
    strncpy(scene->choices[scene->choice_count].action_id, "navi_shutdown", MAX_NAME_LENGTH - 1);
    // No condition for this choice for now
    scene->choice_count++;

    // Choice 2: navi_reboot
    scene->choices[scene->choice_count].text_id = TEXT_CHOICE_NAVI_REBOOT;
    strncpy(scene->choices[scene->choice_count].action_id, "navi_reboot", MAX_NAME_LENGTH - 1);
    // No condition for this choice for now
    scene->choice_count++;

    // Choice 3: navi_connect
    scene->choices[scene->choice_count].text_id = TEXT_CHOICE_NAVI_CONNECT;
    strncpy(scene->choices[scene->choice_count].action_id, "navi_connect", MAX_NAME_LENGTH - 1);
    // No condition for this choice for now
    scene->choice_count++;
}
