#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02c_dad_ask_help(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02c_dad_ask_help", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 5;
    scene->text_content_ids[0] = TEXT_DAD_ASK_HELP_DESC1;
    scene->text_content_ids[1] = TEXT_DAD_ASK_HELP_DAD_QUOTE1;
    scene->text_content_ids[2] = TEXT_DAD_ASK_HELP_DESC2;
    scene->text_content_ids[3] = TEXT_DAD_ASK_HELP_DAD_QUOTE2;
    scene->text_content_ids[4] = TEXT_DAD_ASK_HELP_DESC3;

    // Scene Choices
    scene->choice_count = 1;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_END_HELP_RETURN_TO_LIVING_ROOM;
    strncpy(scene->choices[0].action_id, "return_to_living_room", MAX_NAME_LENGTH - 1);
    scene->choices[0].condition.flag_name[0] = '\0'; // No condition
}
