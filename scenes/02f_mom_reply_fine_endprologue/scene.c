#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02f_mom_reply_fine_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "kurani_residence/living_room");

    scene->text_content_ids[0] = TEXT_MOM_REPLY_FINE_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_MOM_REPLY_FINE_DESC1;
    scene->text_content_ids[3] = TEXT_MOM_REPLY_FINE_MOM_QUOTE1;
    scene->text_content_ids[4] = TEXT_EMPTY_LINE;
    scene->text_content_ids[5] = TEXT_MOM_REPLY_FINE_DESC2;
    scene->text_content_ids[6] = TEXT_EMPTY_LINE;
    scene->text_content_ids[7] = TEXT_MOM_REPLY_FINE_DESC3;
    scene->text_line_count = 8;

    scene->choice_count = 0;
}
