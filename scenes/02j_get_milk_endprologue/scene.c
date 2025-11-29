#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02j_get_milk_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "去拿牛奶");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen"); // Assuming takes place in living room

    scene->text_content_ids[0] = TEXT_GET_MILK_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_GET_MILK_DESC1;
    scene->text_content_ids[3] = TEXT_GET_MILK_DESC2;
    scene->text_content_ids[4] = TEXT_GET_MILK_DESC3;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_GET_MILK_PARENT_QUOTE1;
    scene->text_content_ids[7] = TEXT_GET_MILK_PARENT_QUOTE2;
    scene->text_content_ids[8] = TEXT_GET_MILK_PARENT_QUOTE3;
    scene->text_content_ids[9] = TEXT_GET_MILK_PARENT_QUOTE4;
    scene->text_content_ids[10] = TEXT_EMPTY_LINE;
    scene->text_content_ids[11] = TEXT_GET_MILK_DESC4;
    scene->text_content_ids[12] = TEXT_GET_MILK_DESC5;
    scene->text_content_ids[13] = TEXT_GET_MILK_DESC6;
    scene->text_line_count = 14;

    scene->choice_count = 0;
}
