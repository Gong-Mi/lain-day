#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02c_dad_ask_help(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "父亲请求帮助");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DAD_ASK_HELP_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_DAD, .text_id = TEXT_DAD_ASK_HELP_DAD_QUOTE1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DAD_ASK_HELP_DESC2};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_DAD, .text_id = TEXT_DAD_ASK_HELP_DAD_QUOTE2};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DAD_ASK_HELP_DESC3};
    scene->dialogue_line_count = 8;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_END_HELP_RETURN_TO_LIVING_ROOM, .action_id = "return_to_living_room"};
    scene->choice_count = 1;
}