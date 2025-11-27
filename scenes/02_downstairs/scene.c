#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02_downstairs(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02_downstairs", MAX_NAME_LENGTH - 1);

    // Location ID (from the action that led here)
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 5;
    scene->text_content_ids[0] = TEXT_DOWNSTAIRS_DESC1;
    scene->text_content_ids[1] = TEXT_DOWNSTAIRS_DESC2;
    scene->text_content_ids[2] = TEXT_DOWNSTAIRS_DESC3;
    scene->text_content_ids[3] = TEXT_DOWNSTAIRS_DESC4;
    scene->text_content_ids[4] = TEXT_DOWNSTAIRS_DESC5;

    // Scene Choices
    scene->choice_count = 4;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_TALK_TO_DAD;
    strncpy(scene->choices[0].action_id, "talk_to_dad", MAX_NAME_LENGTH - 1);
    
    // Choice 2
    scene->choices[1].text_id = TEXT_CHOICE_TALK_TO_MOM;
    strncpy(scene->choices[1].action_id, "talk_to_mom", MAX_NAME_LENGTH - 1);

    // Choice 3
    scene->choices[2].text_id = TEXT_CHOICE_GET_MILK;
    strncpy(scene->choices[2].action_id, "get_milk", MAX_NAME_LENGTH - 1);

    // Choice 4
    scene->choices[3].text_id = TEXT_CHOICE_RETURN_TO_UPSTAIRS;
    strncpy(scene->choices[3].action_id, "return_to_upstairs", MAX_NAME_LENGTH - 1);
}
