#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01_lain_room(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "玲音的房间");
    strcpy(scene->location_id, "iwakura_lains_room");

    scene->text_content_ids[0] = TEXT_LAIN_ROOM_TITLE;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_LAIN_ROOM_DESC1;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_LAIN_ROOM_DESC2;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_LAIN_ROOM_DESC3;
    scene->text_content_ids[7] = TEXT_EMPTY_LINE;
    scene->text_content_ids[8] = TEXT_LAIN_ROOM_DESC4;
    scene->text_line_count = 9;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_FIGURE, .action_id = "talk_to_figure"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_EXAMINE_NAVI, .action_id = "examine_navi"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_LEAVE_ROOM, .action_id = "return_to_entry"};
    scene->choice_count = 3;
}