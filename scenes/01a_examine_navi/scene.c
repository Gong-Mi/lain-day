#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01a_examine_navi(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "检查导航器");
    strcpy(scene->location_id, "iwakura_lains_room");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC3};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC4};
    scene->dialogue_line_count = 6;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_SHUTDOWN, .action_id = "navi_shutdown"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_REBOOT, .action_id = "navi_reboot"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_CONNECT, .action_id = "navi_connect"};
    scene->choice_count = 3;
}