#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00a_wait_one_minute_endprologue(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "00a_wait_one_minute_endprologue", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "entry", MAX_NAME_LENGTH - 1); // Implicitly still in the entry room

    // Set scene text
    scene->text_content_ids[0] = TEXT_WAIT_DESC1;
    scene->text_content_ids[1] = TEXT_WAIT_DESC2;
    scene->text_content_ids[2] = TEXT_WAIT_DESC3;
    scene->text_content_ids[3] = TEXT_WAIT_DESC4;
    scene->text_content_ids[4] = TEXT_WAIT_DESC5;
    scene->text_content_ids[5] = TEXT_WAIT_QUOTE;
    scene->text_line_count = 6;

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_START_CHAPTER_ONE;
    strncpy(scene->choices[0].action_id, "start_chapter_one", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 1;
}
