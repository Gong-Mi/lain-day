#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_wait_nothing(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strncpy(scene->scene_id, "SCENE_WAIT_DOES_NOTHING", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "iwakura_upper_hallway", MAX_NAME_LENGTH - 1);

    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_WAIT_NOTHING_DESC1};
    scene->dialogue_line_count = 1;

    scene->choices[0] = (StoryChoice){TEXT_CHOICE_RETURN_TO_UPPER_HALLWAY, "return_to_upstairs", {0}};
    scene->choice_count = 1;
}
