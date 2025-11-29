#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00_entry(StoryScene* scene) {
    // Clear previous state
        memset(scene, 0, sizeof(StoryScene));
        strcpy(scene->name, "初始空间");
        // The title "初始空间" is usually handled by a different UI element or omitted,
    // as it's not part of the sequential text content in the string table.
    scene->text_content_ids[0] = TEXT_EMPTY_LINE;
    scene->text_content_ids[1] = TEXT_ENTRY_DESC1;
    scene->text_content_ids[2] = TEXT_ENTRY_DESC2;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_ENTRY_DESC3;
    scene->text_content_ids[5] = TEXT_EMPTY_LINE;
    scene->text_content_ids[6] = TEXT_ENTRY_DESC4;
    scene->text_content_ids[7] = TEXT_EMPTY_LINE;
    scene->text_content_ids[8] = TEXT_ENTRY_DESC5;
    scene->text_line_count = 9;

    // Choices - Mapped directly to pre-defined StringIDs
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_OPEN_DOOR, .action_id = "enter_lain_room"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_GO_DOWNSTAIRS, .action_id = "go_downstairs"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_WAIT_ONE_MINUTE, .action_id = "wait_one_minute"};
    scene->choice_count = 3;
}