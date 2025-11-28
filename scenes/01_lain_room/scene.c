#include "scene.h"
#include "game_types.h" // Required for StoryScene and StoryChoice types
#include "string_ids.h" // Required for StringID enum values
#include <string.h>

void init_scene_01_lain_room(StoryScene* scene) {
    // Clear previous state
    memset(scene, 0, sizeof(StoryScene));

    // Set the location metadata
    strcpy(scene->location_id, "lain_room"); // Corrected: use scene->location_id

    // Text Content - Map directly to pre-defined StringIDs
    scene->text_content_ids[0] = TEXT_LAIN_ROOM_TITLE; // "lain的房间"
    scene->text_content_ids[1] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[2] = TEXT_LAIN_ROOM_DESC1; // "你推开门，向前走了一步，房间内的布局清晰地展现在眼前。"
    scene->text_content_ids[3] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[4] = TEXT_LAIN_ROOM_DESC2; // "房间里很暗。一张巨大的书桌占据了角落，靠着两面墙，上面堆满了书籍和设备。在书堆之下，一台老旧的 `Navi` 屏幕亮着幽光。"
    scene->text_content_ids[5] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[6] = TEXT_LAIN_ROOM_DESC3; // "房间的中央是一个飘窗。你的右边是一张靠墙的床，床和飘窗之间散落着几个布娃娃。"
    scene->text_content_ids[7] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[8] = TEXT_LAIN_ROOM_DESC4; // "而在飘窗上，有一个人影正靠着窗户，静静地坐在窗沿上。"
    scene->text_line_count = 9;

    // Choices - Map directly to pre-defined StringIDs
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_FIGURE, .action_id = "talk_to_figure"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_EXAMINE_NAVI, .action_id = "examine_navi"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_LEAVE_ROOM, .action_id = "return_to_entry"}; // Corresponds to "默默地离开房间"
    scene->choice_count = 3;
}