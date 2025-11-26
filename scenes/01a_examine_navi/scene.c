#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01a_examine_navi(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "01a_examine_navi", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "lain_room", MAX_NAME_LENGTH - 1); // This scene occurs within lain_room

    // Set scene text
    scene->text_content_ids[0] = TEXT_NAVI_STATE_TITLE; // Assuming it's still used as text
    scene->text_content_ids[1] = TEXT_NAVI_STATE_DESC1;
    scene->text_content_ids[2] = TEXT_NAVI_STATE_DESC2;
    scene->text_content_ids[3] = TEXT_NAVI_STATE_DESC3;
    scene->text_content_ids[4] = TEXT_NAVI_STATE_DESC4;
    scene->text_line_count = 5; // Title + 4 descriptions

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_NAVI_SHUTDOWN;
    strncpy(scene->choices[0].action_id, "navi_shutdown", MAX_NAME_LENGTH - 1);

    scene->choices[1].text_id = TEXT_CHOICE_NAVI_REBOOT;
    strncpy(scene->choices[1].action_id, "navi_reboot", MAX_NAME_LENGTH - 1);

    scene->choices[2].text_id = TEXT_CHOICE_NAVI_CONNECT;
    strncpy(scene->choices[2].action_id, "navi_connect", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 3;
}
