#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01b_navi_shutdown(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "导航器关机");
    strcpy(scene->location_id, "iwakura_lains_room");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_SHUTDOWN_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_SHUTDOWN_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_SHUTDOWN_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_SHUTDOWN_DESC3};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_SHUTDOWN_DESC4};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_NAVI_SHUTDOWN_QUOTE};
    scene->dialogue_line_count = 8;

    // This is a narrative scene with no choices. The game will likely
    // either end or automatically transition after displaying the text.
    scene->choice_count = 0; 
}