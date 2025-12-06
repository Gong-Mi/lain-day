#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01a_examine_navi(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->name, "检查导航器");
    strcpy(scene->location_id, "iwakura_lains_room");

    // Get game state to access the style counter
    GameState* game_state = g_game_state_ptr;

    // Define the animation sequence
    static const StringID progress_styles[] = {
        TEXT_NAVI_PROGRESS_1,
        TEXT_NAVI_PROGRESS_2,
        TEXT_NAVI_PROGRESS_3,
        TEXT_NAVI_PROGRESS_4
    };

    // Determine which style to show
    StringID current_progress_style_id = progress_styles[game_state->navi_progress_style];

    // Update the style for the next refresh
    game_state->navi_progress_style = (game_state->navi_progress_style + 1) % 4;

    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_TITLE};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_EMPTY_LINE};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC1};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC2};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = current_progress_style_id}; // Use dynamic style
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_NAVI_STATE_DESC4};
    scene->dialogue_line_count = 6;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_SHUTDOWN, .action_id = "navi_shutdown"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_REBOOT, .action_id = "navi_reboot"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_CONNECT, .action_id = "navi_connect"};
    scene->choices[3] = (StoryChoice){.text_id = TEXT_CHOICE_NAVI_REFRESH, .action_id = "refresh_navi_screen"};
    scene->choices[4] = (StoryChoice){.text_id = TEXT_CHOICE_LEAVE_ROOM, .action_id = "exit_room"}; // Assuming exit_room is generic
    scene->choice_count = 5;
}