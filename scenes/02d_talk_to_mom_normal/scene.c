#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02d_talk_to_mom_normal(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "02d_talk_to_mom_normal", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text
    scene->text_line_count = 3;
    scene->text_content_ids[0] = TEXT_TALK_TO_MOM_NORMAL_DESC1;
    scene->text_content_ids[1] = TEXT_TALK_TO_MOM_NORMAL_MOM_QUOTE1;
    scene->text_content_ids[2] = TEXT_TALK_TO_MOM_NORMAL_DESC2;

    // Scene Choices
    scene->choice_count = 2;
    
    // Choice 1
    scene->choices[0].text_id = TEXT_CHOICE_MOM_REPLY_FINE;
    strncpy(scene->choices[0].action_id, "mom_reply_fine", MAX_NAME_LENGTH - 1);
    
    // Choice 2
    scene->choices[1].text_id = TEXT_CHOICE_MOM_REPLY_SILENT;
    strncpy(scene->choices[1].action_id, "mom_reply_silent", MAX_NAME_LENGTH - 1);
}
