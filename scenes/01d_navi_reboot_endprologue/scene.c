#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01d_navi_reboot_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "lain_room");

    scene->text_content_ids[0] = TEXT_NAVI_REBOOT_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_NAVI_REBOOT_DESC1;
    scene->text_content_ids[3] = TEXT_NAVI_REBOOT_DESC2;
    scene->text_content_ids[4] = TEXT_EMPTY_LINE;
    scene->text_content_ids[5] = TEXT_NAVI_REBOOT_YOU_QUOTE1;
    scene->text_content_ids[6] = TEXT_NAVI_REBOOT_DESC3;
    scene->text_content_ids[7] = TEXT_EMPTY_LINE;
    scene->text_content_ids[8] = TEXT_NAVI_REBOOT_GHOST_QUOTE1;
    scene->text_content_ids[9] = TEXT_NAVI_REBOOT_GHOST_QUOTE2;
    scene->text_content_ids[10] = TEXT_NAVI_REBOOT_YOU_QUOTE2;
    scene->text_content_ids[11] = TEXT_NAVI_REBOOT_GHOST_QUOTE3;
    scene->text_content_ids[12] = TEXT_NAVI_REBOOT_YOU_QUOTE3;
    scene->text_content_ids[13] = TEXT_NAVI_REBOOT_DESC4;
    scene->text_content_ids[14] = TEXT_NAVI_REBOOT_GHOST_QUOTE4;
    scene->text_line_count = 15;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_START_CHAPTER_ONE, .action_id = "start_chapter_one"};
    scene->choice_count = 1;
}