#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01a_examine_navi(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "lain_room");

    scene->text_content_ids[0] = TEXT_NAVI_STATE_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_NAVI_STATE_DESC1;
    scene->text_content_ids[3] = TEXT_NAVI_STATE_DESC2;
    scene->text_content_ids[4] = TEXT_NAVI_STATE_DESC3;
    scene->text_content_ids[5] = TEXT_NAVI_STATE_DESC4;
    scene->text_line_count = 6;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_SHUTDOWN, .action_id = "navi_shutdown"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_REBOOT, .action_id = "navi_reboot"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_CONNECT, .action_id = "navi_connect"};
    scene->choice_count = 3;
}