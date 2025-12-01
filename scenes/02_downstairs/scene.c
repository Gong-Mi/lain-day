#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>
#include <stdio.h>

void init_scene_02_downstairs(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "楼下的客厅与厨房");
    strcpy(scene->scene_id, "story/02_downstairs.md");

#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: init_scene_02_downstairs: TEXT_DOWNSTAIRS_DESC1 value: %d\n", TEXT_DOWNSTAIRS_DESC1);
#endif

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DOWNSTAIRS_DESC1};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DOWNSTAIRS_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DOWNSTAIRS_DESC3};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DOWNSTAIRS_DESC4};
    scene->dialogue_lines[8] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[9] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_DOWNSTAIRS_DESC5};
    scene->dialogue_line_count = 10;
    scene->choice_count = 4;
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: init_scene_02_downstairs: scene->dialogue_lines[1].text_id after assignment: %d\n", scene->dialogue_lines[1].text_id);
#endif
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_DAD, .action_id = "talk_to_dad"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_MOM, .action_id = "talk_to_mom"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_GET_MILK, .action_id = "get_milk"};
    scene->choices[3] = (StoryChoice){.text_id = TEXT_CHOICE_RETURN_TO_UPSTAIRS, .action_id = "return_to_upstairs"};
}