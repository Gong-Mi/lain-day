#include "scene.h"
#include "game_types.h" // Required for StoryScene and StoryChoice types
#include "string_ids.h" // Required for StringID enum values
#include <string.h>

void init_scene_00_entry(StoryScene* scene) {
    // Clear previous state
    memset(scene, 0, sizeof(StoryScene));

    // Text Content - Map directly to pre-defined StringIDs
    scene->text_content_ids[0] = TEXT_ENTRY_DESC1; // "初始空间"
    scene->text_content_ids[1] = TEXT_INVALID;     // Empty line for spacing (original)
    scene->text_content_ids[2] = TEXT_ENTRY_DESC2; // "你站在一个空旷、安静的房间里。"
    scene->text_content_ids[3] = TEXT_ENTRY_DESC3; // "房间里几乎没有东西，只有一扇朴素的门，以及通往楼下的楼梯。"
    scene->text_content_ids[4] = TEXT_INVALID;     // Empty line for spacing (original)
    scene->text_content_ids[5] = TEXT_ENTRY_DESC4; // "门上用手写体刻着一个词："
    scene->text_content_ids[6] = TEXT_INVALID;     // Empty line for spacing (original)
    scene->text_content_ids[7] = TEXT_ENTRY_DESC5; // "`lain`"
    scene->text_content_ids[8] = TEXT_INVALID;     // Empty line for spacing (original)
    // "但底下的汉字被抹去了" - This line does not have a corresponding StringID, so it cannot be included directly.
    scene->text_line_count = 9; // Only 9 lines are directly mappable/fillable.

    // Choices - Map directly to pre-defined StringIDs
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_OPEN_DOOR, .action_id = "enter_lain_room"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_GO_DOWNSTAIRS, .action_id = "go_downstairs"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_WAIT_ONE_MINUTE, .action_id = "wait_one_minute"};
    scene->choice_count = 3;
}