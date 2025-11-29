#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_03_chapter_one_intro(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "第一章介绍");
    strcpy(scene->location_id, "iwakura_lains_room"); // Or 'entry' depending on context

    scene->text_content_ids[0] = TEXT_CH1_INTRO_DESC1;
    scene->text_content_ids[1] = TEXT_CH1_INTRO_YOU_QUOTE1;
    scene->text_content_ids[2] = TEXT_EMPTY_LINE;
    scene->text_content_ids[3] = TEXT_CH1_INTRO_DESC2;
    scene->text_content_ids[4] = TEXT_CH1_INTRO_YOU_QUOTE2;
    scene->text_content_ids[5] = TEXT_CH1_INTRO_NAVI_QUOTE1;
    scene->text_content_ids[6] = TEXT_EMPTY_LINE;
    scene->text_content_ids[7] = TEXT_CH1_INTRO_DESC3;
    scene->text_content_ids[8] = TEXT_CH1_INTRO_DESC4;
    scene->text_line_count = 9;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_SISTER, .action_id = "talk_to_sister"};
    scene->choice_count = 1;
}