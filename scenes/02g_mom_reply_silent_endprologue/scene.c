#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02g_mom_reply_silent_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "母亲的回应：沉默");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->text_content_ids[0] = TEXT_MOM_REPLY_SILENT_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_MOM_REPLY_SILENT_DESC1;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_MOM_REPLY_SILENT_MOM_QUOTE1;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_MOM_REPLY_SILENT_DESC2;
    scene->text_line_count = 7;

    scene->choice_count = 0;
}
