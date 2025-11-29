#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02d_talk_to_mom_normal(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->text_content_ids[0] = TEXT_TALK_TO_MOM_NORMAL_DESC1;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_TALK_TO_MOM_NORMAL_MOM_QUOTE1;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_TALK_TO_MOM_NORMAL_DESC2;
    scene->text_line_count = 5;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_MOM_REPLY_FINE, .action_id = "mom_reply_fine"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_MOM_REPLY_SILENT, .action_id = "mom_reply_silent"};
    scene->choice_count = 2;
}