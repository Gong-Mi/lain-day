#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01b_navi_shutdown(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "lain_room");

    scene->text_content_ids[0] = TEXT_NAVI_SHUTDOWN_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_NAVI_SHUTDOWN_DESC1;
    scene->text_content_ids[3] = TEXT_NAVI_SHUTDOWN_DESC2;
    scene->text_content_ids[4] = TEXT_NAVI_SHUTDOWN_DESC3;
    scene->text_content_ids[5] = TEXT_NAVI_SHUTDOWN_DESC4;
    scene->text_content_ids[6] = TEXT_EMPTY_LINE;
    scene->text_content_ids[7] = TEXT_NAVI_SHUTDOWN_QUOTE;
    scene->text_line_count = 8;

    // This is a narrative scene with no choices. The game will likely
    // either end or automatically transition after displaying the text.
    scene->choice_count = 0; 
}