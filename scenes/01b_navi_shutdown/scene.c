#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01b_navi_shutdown(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "01b_navi_shutdown", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "lain_room", MAX_NAME_LENGTH - 1); // This scene occurs within lain_room

    // Set scene text
    scene->text_content_ids[0] = TEXT_NAVI_SHUTDOWN_DESC1;
    scene->text_content_ids[1] = TEXT_NAVI_SHUTDOWN_DESC2;
    scene->text_content_ids[2] = TEXT_NAVI_SHUTDOWN_DESC3;
    scene->text_content_ids[3] = TEXT_NAVI_SHUTDOWN_DESC4;
    scene->text_content_ids[4] = TEXT_NAVI_SHUTDOWN_QUOTE;
    scene->text_line_count = 5; 

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_GO_DOWNSTAIRS;
    strncpy(scene->choices[0].action_id, "go_downstairs", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 1;
}
