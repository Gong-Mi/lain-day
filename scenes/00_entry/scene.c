#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00_entry(StoryScene* scene) {
    // Clear previous state
            memset(scene, 0, sizeof(StoryScene));
            strcpy(scene->name, "初始空间");
            strcpy(scene->scene_id, "story/00_entry.md");        // The title "初始空间" is usually handled by a different UI element or omitted,
    // as it's not part of the sequential text content in the string table.
    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_ENTRY_DESC1};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_ENTRY_DESC2};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_ENTRY_DESC3};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_ENTRY_DESC4};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[8] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_ENTRY_DESC5};
    scene->dialogue_line_count = 9;

    // Choices - Mapped directly to pre-defined StringIDs
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_OPEN_DOOR, .action_id = "enter_lain_room"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_GO_DOWNSTAIRS, .action_id = "go_downstairs"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_WAIT_ONE_MINUTE, .action_id = "wait_one_minute"};
    scene->choice_count = 3;
}