#include "scene.h"
#include "string_ids.h" // For the new text IDs
#include <string.h>

void init_scene_examine_fridge(StoryScene* scene) {
    // Scene metadata
    strcpy(scene->scene_id, "SCENE_EXAMINE_FRIDGE");
    strcpy(scene->name, "Examine Refrigerator");
    strcpy(scene->location_id, "iwakura_living_dining_kitchen");

    // Dialogue and description
    scene->dialogue_line_count = 1;
    scene->dialogue_lines[0] = (DialogueLine){SPEAKER_NONE, TEXT_EXAMINE_FRIDGE_DESC1};

    // Choices
    scene->choice_count = 2;
    scene->choices[0] = (StoryChoice){TEXT_CHOICE_TAKE_MILK, "take_milk_from_fridge", {}};
    scene->choices[1] = (StoryChoice){TEXT_CHOICE_CLOSE_FRIDGE, "return_to_living_room", {}};
}
