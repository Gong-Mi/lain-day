#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00a_wait_one_minute_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    // This scene likely takes place in the 'entry' location.
    strcpy(scene->location_id, "entry");

    scene->text_content_ids[0] = TEXT_WAIT_DESC1;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_WAIT_DESC2;
    scene->text_content_ids[3] = TEXT_WAIT_DESC3;
    scene->text_content_ids[4] = TEXT_WAIT_DESC4;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_WAIT_DESC5;
    scene->text_content_ids[7] = TEXT_WAIT_QUOTE;
    scene->text_line_count = 8;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_START_CHAPTER_ONE, .action_id = "start_chapter_one"};
    scene->choice_count = 1;
}