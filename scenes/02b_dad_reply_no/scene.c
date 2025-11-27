#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02b_dad_reply_no(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02b_dad_reply_no", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 3;
    scene->text_content_ids[0] = TEXT_DAD_REPLY_NO_DESC1;
    scene->text_content_ids[1] = TEXT_DAD_REPLY_NO_DAD_QUOTE1;
    scene->text_content_ids[2] = TEXT_DAD_REPLY_NO_DESC2;

    // Scene Choices
    scene->choice_count = 1;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_RETURN_TO_LIVING_ROOM_FROM_DAD;
    strncpy(scene->choices[0].action_id, "return_to_living_room", MAX_NAME_LENGTH - 1);
    scene->choices[0].condition.flag_name[0] = '\0'; // No condition
}
