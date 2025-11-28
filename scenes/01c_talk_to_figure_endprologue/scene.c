#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01c_talk_to_figure_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "lain_room");

    scene->text_content_ids[0] = TEXT_TALK_FIGURE_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_TALK_FIGURE_DESC1;
    scene->text_content_ids[3] = TEXT_TALK_FIGURE_YOU_QUOTE1;
    scene->text_content_ids[4] = TEXT_EMPTY_LINE;
    scene->text_content_ids[5] = TEXT_TALK_FIGURE_DESC2;
    scene->text_content_ids[6] = TEXT_TALK_FIGURE_GHOST_QUOTE1;
    scene->text_content_ids[7] = TEXT_TALK_FIGURE_GHOST_QUOTE2;
    scene->text_content_ids[8] = TEXT_TALK_FIGURE_YOU_QUOTE2;
    scene->text_content_ids[9] = TEXT_TALK_FIGURE_GHOST_QUOTE3;
    scene->text_content_ids[10] = TEXT_TALK_FIGURE_YOU_QUOTE3;
    scene->text_content_ids[11] = TEXT_EMPTY_LINE;
    scene->text_content_ids[12] = TEXT_TALK_FIGURE_DESC3;
    scene->text_line_count = 13;

    scene->choice_count = 0;
}