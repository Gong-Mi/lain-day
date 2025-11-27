#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_03_chapter_one_intro(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "03_chapter_one_intro", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 7;
    scene->text_content_ids[0] = TEXT_CH1_INTRO_DESC1;
    scene->text_content_ids[1] = TEXT_CH1_INTRO_YOU_QUOTE1;
    scene->text_content_ids[2] = TEXT_CH1_INTRO_DESC2;
    scene->text_content_ids[3] = TEXT_CH1_INTRO_YOU_QUOTE2;
    scene->text_content_ids[4] = TEXT_CH1_INTRO_NAVI_QUOTE1;
    scene->text_content_ids[5] = TEXT_CH1_INTRO_DESC3;
    scene->text_content_ids[6] = TEXT_CH1_INTRO_DESC4;

    // Scene Choices
    scene->choice_count = 1;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_TALK_TO_SISTER;
    strncpy(scene->choices[0].action_id, "talk_to_sister", MAX_NAME_LENGTH - 1);
}
