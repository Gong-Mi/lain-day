#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_00a_wait_one_minute_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    // This scene likely takes place in the 'entry' location.
    strcpy(scene->location_id, "entry");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_WAIT_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_WAIT_DESC2};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_WAIT_DESC3};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_WAIT_DESC4};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_WAIT_DESC5};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_MIRA, .text_id = TEXT_WAIT_QUOTE}; // Assuming Mika is the speaker
    scene->dialogue_line_count = 8;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_START_CHAPTER_ONE, .action_id = "start_chapter_one"};
    scene->choice_count = 1;
}