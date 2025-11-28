#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02b_dad_reply_no(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "kurani_residence/living_room");

    scene->text_content_ids[0] = TEXT_DAD_REPLY_NO_DESC1;
    scene->text_content_ids[1] = TEXT_EMPTY_LINE;
    scene->text_content_ids[2] = TEXT_DAD_REPLY_NO_DAD_QUOTE1;
    scene->text_content_ids[3] = TEXT_EMPTY_LINE;
    scene->text_content_ids[4] = TEXT_DAD_REPLY_NO_DESC2;
    scene->text_line_count = 5;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_RETURN_TO_LIVING_ROOM_FROM_DAD, .action_id = "return_to_living_room"};
    scene->choice_count = 1;
}
