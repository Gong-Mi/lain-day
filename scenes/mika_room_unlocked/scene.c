#include "scene.h"
#include "string_ids.h"
#include <string.h>
#include "scenes.h" // For SCENE_MIKA_ROOM_UNLOCKED
#include "characters/mika.h" // For get_mika_module()
#include "flag_system.h" // For hash_table_get
#include "game_types.h" // For g_game_state_ptr

void init_scene_mika_room_unlocked(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strncpy(scene->scene_id, "SCENE_MIKA_ROOM_UNLOCKED", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "iwakura_mikas_room", MAX_NAME_LENGTH - 1);

    const CharacterMika* mika = get_mika_module();
    const GameState* game_state = g_game_state_ptr; // Use the global pointer
    
    // Dialogue
    if (game_state != NULL && strcmp(mika->current_location_id, scene->location_id) == 0) {
        // Mika is in her room
        scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_MIKA_ROOM_UNLOCKED_DESC1};
        scene->dialogue_lines[1] = (DialogueLine){SPEAKER_MIRA, TEXT_MIKA_ROOM_UNLOCKED_MIRA_QUOTE1};
        scene->dialogue_line_count = 2;

        // Choices
        scene->choices[0] = (StoryChoice){TEXT_CHOICE_MIKA_ROOM_JUST_LOOKING, "exit_room", {0}}; 
        scene->choices[1] = (StoryChoice){TEXT_CHOICE_MIKA_ROOM_TALK, "talk_to_sister", {0}};
        scene->choice_count = 2;
    } else {
        // Mika is not in her room or game_state is null
        scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_MIKA_ROOM_UNLOCKED_EMPTY_DESC1};
        scene->dialogue_line_count = 1;

        // Choices (only exit_room)
        scene->choices[0] = (StoryChoice){TEXT_CHOICE_MIKA_ROOM_JUST_LOOKING, "exit_room", {0}}; 
        scene->choice_count = 1;
    }
}
