#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>
#include <stdio.h>

void init_scene_01_lain_room(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "玲音的房间");
    strcpy(scene->scene_id, "story/01_lain_room.md");
    strcpy(scene->location_id, "iwakura_lains_room");

#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: init_scene_01_lain_room: TEXT_LAIN_ROOM_TITLE value: %d\n", TEXT_LAIN_ROOM_TITLE);
#endif

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_DESC2};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_DESC3};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[8] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_DESC4};
    scene->dialogue_lines[9] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_LAIN_ROOM_DESC5}; // TEXT_LAIN_ROOM_DESC5 was not part of the old block
    scene->dialogue_line_count = 10;
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_FIGURE, .action_id = "talk_to_figure"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_EXAMINE_NAVI, .action_id = "examine_navi"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_LEAVE_ROOM, .action_id = "exit_room"};
    scene->choice_count = 3;
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: init_scene_01_lain_room: scene->dialogue_lines[0].text_id after assignment: %d\n", scene->dialogue_lines[0].text_id);
#endif
}