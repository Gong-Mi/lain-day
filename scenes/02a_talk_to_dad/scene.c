#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02a_talk_to_dad(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "kurani_residence/living_room");

    scene->text_content_ids[0] = TEXT_TALK_TO_DAD_DESC1;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_TALK_TO_DAD_DAD_QUOTE1;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_TALK_TO_DAD_DESC2;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_TALK_TO_DAD_DESC3;
    scene->text_line_count = 7;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_REPLY_NO, .action_id = "dad_reply_no"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_ASK_UPGRADE, .action_id = "dad_ask_upgrade"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_ASK_HELP, .action_id = "dad_ask_help"};
    scene->choice_count = 3;
}