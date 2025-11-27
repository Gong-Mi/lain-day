#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02a_talk_to_dad(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02a_talk_to_dad", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 4;
    scene->text_content_ids[0] = TEXT_TALK_TO_DAD_DESC1;
    scene->text_content_ids[1] = TEXT_TALK_TO_DAD_DAD_QUOTE1;
    scene->text_content_ids[2] = TEXT_TALK_TO_DAD_DESC2;
    scene->text_content_ids[3] = TEXT_TALK_TO_DAD_DESC3;

    // Scene Choices
    scene->choice_count = 3;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_DAD_REPLY_NO;
    strncpy(scene->choices[0].action_id, "dad_reply_no", MAX_NAME_LENGTH - 1);
    scene->choices[0].condition.flag_name[0] = '\0'; // No condition

    // Choice 2 (Unselectable for now)
    scene->choices[1].text_id = TEXT_CHOICE_DAD_ASK_UPGRADE;
    strncpy(scene->choices[1].action_id, "dad_ask_upgrade", MAX_NAME_LENGTH - 1);
    // This choice requires a flag `can_upgrade_navi` to be 1.
    // This makes it effectively unselectable until the flag is set.
    strncpy(scene->choices[1].condition.flag_name, "can_upgrade_navi", MAX_NAME_LENGTH - 1);
    scene->choices[1].condition.required_value = 1;

    // Choice 3
    scene->choices[2].text_id = TEXT_CHOICE_DAD_ASK_HELP;
    strncpy(scene->choices[2].action_id, "dad_ask_help", MAX_NAME_LENGTH - 1);
    scene->choices[2].condition.flag_name[0] = '\0'; // No condition
}
