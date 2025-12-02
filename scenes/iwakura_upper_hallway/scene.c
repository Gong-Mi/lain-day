#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_iwakura_upper_hallway(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strncpy(scene->scene_id, "SCENE_IWAKURA_UPPER_HALLWAY", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "iwakura_upper_hallway", MAX_NAME_LENGTH - 1);

    // Dialogue
    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_UPPER_HALLWAY_DESC1};
    scene->dialogue_line_count = 1;

    // Choices (will be dynamically generated based on connections)
    // For now, let's add some dummy choices
    scene->choices[0] = (StoryChoice){TEXT_CHOICE_ENTER_LAINS_ROOM, "enter_lains_room", {0}};
    scene->choices[1] = (StoryChoice){TEXT_CHOICE_ENTER_MIKAS_ROOM, "enter_mikas_room", {0}};
    scene->choices[2] = (StoryChoice){TEXT_CHOICE_GO_DOWNSTAIRS_FROM_UPPER_HALLWAY, "go_downstairs", {0}};
    scene->choice_count = 3;
}
