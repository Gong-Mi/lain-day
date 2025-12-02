#include "scene.h"
#include "string_ids.h"
#include <string.h>
#include "scenes.h" // For SCENE_MIKA_ROOM_UNLOCKED

void init_scene_mika_room_unlocked(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strncpy(scene->scene_id, "SCENE_MIKA_ROOM_UNLOCKED", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "iwakura_mikas_room", MAX_NAME_LENGTH - 1);

    // Dialogue
    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_MIKA_ROOM_UNLOCKED_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){SPEAKER_MIRA, TEXT_MIKA_ROOM_UNLOCKED_MIRA_QUOTE1};
    scene->dialogue_line_count = 2;

    // Choices
    scene->choices[0] = (StoryChoice){TEXT_CHOICE_MIKA_ROOM_JUST_LOOKING, "exit_room", {0}}; // exit_room takes player to upper_hallway
    scene->choices[1] = (StoryChoice){TEXT_CHOICE_MIKA_ROOM_TALK, "talk_to_sister", {0}};
    scene->choice_count = 2;
}
