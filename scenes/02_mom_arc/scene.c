#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02d_talk_to_mom_normal(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");
    strcpy(scene->scene_id, "story/02d_talk_to_mom_normal.md");
    strcpy(scene->name, "和妈妈对话：正常");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_TALK_TO_MOM_NORMAL_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_MOM, .text_id = TEXT_TALK_TO_MOM_NORMAL_MOM_QUOTE1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_TALK_TO_MOM_NORMAL_DESC2};
    scene->dialogue_line_count = 5;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_MOM_REPLY_FINE, .action_id = "mom_reply_fine"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_MOM_REPLY_SILENT, .action_id = "mom_reply_silent"};
    scene->choice_count = 2;
}

void init_scene_02f_mom_reply_fine_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "母亲的回应：还好");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_FINE_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_FINE_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_MOM, .text_id = TEXT_MOM_REPLY_FINE_MOM_QUOTE1};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_FINE_DESC2};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_FINE_DESC3};
    scene->dialogue_line_count = 8;

    scene->choice_count = 0;
}

void init_scene_02g_mom_reply_silent_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "母亲的回应：沉默");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_SILENT_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_SILENT_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_MOM, .text_id = TEXT_MOM_REPLY_SILENT_MOM_QUOTE1};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_MOM_REPLY_SILENT_DESC2};
    scene->dialogue_line_count = 7;

    scene->choice_count = 0;
}
