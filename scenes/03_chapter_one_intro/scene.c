#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_03_chapter_one_intro(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "第一章介绍");
    strcpy(scene->location_id, "iwakura_lains_room"); // Or 'entry' depending on context

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_CH1_INTRO_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_CH1_INTRO_YOU_QUOTE1};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_CH1_INTRO_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_CH1_INTRO_YOU_QUOTE2};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NAVI, .text_id = TEXT_CH1_INTRO_NAVI_QUOTE1};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_CH1_INTRO_DESC3};
    scene->dialogue_lines[8] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_CH1_INTRO_DESC4};
    scene->dialogue_line_count = 9;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_SISTER, .action_id = "talk_to_sister"};
    scene->choice_count = 1;
}