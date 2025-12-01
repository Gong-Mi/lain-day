#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01d_navi_reboot_endprologue(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "导航器重启");
    strcpy(scene->location_id, "iwakura_lains_room");

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_REBOOT_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_REBOOT_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_REBOOT_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_NAVI_REBOOT_YOU_QUOTE1};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_REBOOT_DESC3};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[8] = (DialogueLine){.speaker_id = SPEAKER_GHOST, .text_id = TEXT_NAVI_REBOOT_GHOST_QUOTE1};
    scene->dialogue_lines[9] = (DialogueLine){.speaker_id = SPEAKER_GHOST, .text_id = TEXT_NAVI_REBOOT_GHOST_QUOTE2};
    scene->dialogue_lines[10] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_NAVI_REBOOT_YOU_QUOTE2};
    scene->dialogue_lines[11] = (DialogueLine){.speaker_id = SPEAKER_GHOST, .text_id = TEXT_NAVI_REBOOT_GHOST_QUOTE3};
    scene->dialogue_lines[12] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_NAVI_REBOOT_YOU_QUOTE3};
    scene->dialogue_lines[13] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_REBOOT_DESC4};
    scene->dialogue_lines[14] = (DialogueLine){.speaker_id = SPEAKER_GHOST, .text_id = TEXT_NAVI_REBOOT_GHOST_QUOTE4};
    scene->dialogue_line_count = 15;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_START_CHAPTER_ONE, .action_id = "start_chapter_one"};
    scene->choice_count = 1;
}