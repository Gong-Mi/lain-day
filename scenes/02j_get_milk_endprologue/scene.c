#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02j_get_milk_endprologue(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02j_get_milk_endprologue", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "entry", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 8;
    scene->text_content_ids[0] = TEXT_GET_MILK_DESC1;
    scene->text_content_ids[1] = TEXT_GET_MILK_DESC2;
    scene->text_content_ids[2] = TEXT_GET_MILK_DESC3;
    scene->text_content_ids[3] = TEXT_GET_MILK_PARENT_QUOTE1;
    scene->text_content_ids[4] = TEXT_GET_MILK_PARENT_QUOTE2;
    scene->text_content_ids[5] = TEXT_GET_MILK_PARENT_QUOTE3;
    scene->text_content_ids[6] = TEXT_GET_MILK_PARENT_QUOTE4;
    scene->text_content_ids[7] = TEXT_GET_MILK_DESC4; // Re-using for the final line of description

    // Scene Choices
    scene->choice_count = 1;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_START_CHAPTER_ONE;
    strncpy(scene->choices[0].action_id, "start_chapter_one", MAX_NAME_LENGTH - 1);
    scene->choices[0].condition.flag_name[0] = '\0'; // No condition
}
