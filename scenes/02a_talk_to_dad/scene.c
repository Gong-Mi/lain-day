#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02a_talk_to_dad(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "与父亲对话");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_TALK_TO_DAD_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_DAD, .text_id = TEXT_TALK_TO_DAD_DAD_QUOTE1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_TALK_TO_DAD_DESC2};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_TALK_TO_DAD_DESC3};
    scene->dialogue_line_count = 7;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_REPLY_NO, .action_id = "dad_reply_no"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_ASK_UPGRADE, .action_id = "dad_ask_upgrade"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_DAD_ASK_HELP, .action_id = "dad_ask_help"};
    scene->choice_count = 3;
}