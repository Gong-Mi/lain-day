#include "scene.h"
#include "string_ids.h"
#include <string.h>
#include "scenes.h" // For SCENE_MIKA_ROOM_LOCKED

void init_scene_mika_room_locked(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strncpy(scene->scene_id, "SCENE_MIKA_ROOM_LOCKED", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "iwakura_upper_hallway", MAX_NAME_LENGTH - 1);

    // Dialogue
    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_MIKA_ROOM_LOCKED_DESC1};
    scene->dialogue_line_count = 1;

    // Choices
    scene->choices[0] = (StoryChoice){TEXT_CHOICE_RETURN_TO_UPPER_HALLWAY, "go_to_upper_hallway", {0}};
    scene->choice_count = 1;
}
