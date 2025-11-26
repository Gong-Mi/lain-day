#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01_lain_room(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "01_lain_room", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "lain_room", MAX_NAME_LENGTH - 1);

    // Set scene text
    scene->text_content_ids[0] = TEXT_LAIN_ROOM_DESC1;
    scene->text_content_ids[1] = TEXT_LAIN_ROOM_DESC2;
    scene->text_content_ids[2] = TEXT_LAIN_ROOM_DESC3;
    scene->text_content_ids[3] = TEXT_LAIN_ROOM_DESC4;
    scene->text_content_ids[4] = TEXT_LAIN_ROOM_DESC5;
    scene->text_line_count = 5;

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_TALK_TO_FIGURE;
    strncpy(scene->choices[0].action_id, "talk_to_figure", MAX_NAME_LENGTH - 1);

    scene->choices[1].text_id = TEXT_CHOICE_EXAMINE_NAVI;
    strncpy(scene->choices[1].action_id, "examine_navi", MAX_NAME_LENGTH - 1);

    scene->choices[2].text_id = TEXT_CHOICE_LEAVE_ROOM; // This maps to return_to_entry action
    strncpy(scene->choices[2].action_id, "return_to_entry", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 3;
}
