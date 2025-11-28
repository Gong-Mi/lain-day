#include "scene.h"
#include "game_types.h"
#include "string_ids.h"
#include <string.h>

void init_scene_02_downstairs(StoryScene* scene) {
    memset(scene, 0, sizeof(StoryScene));
    strcpy(scene->location_id, "kurani_residence/living_room");

    // The title "楼下的客厅与厨房" has no StringID, so it's omitted.
    scene->text_content_ids[0] = TEXT_EMPTY_LINE;
    scene->text_content_ids[1] = TEXT_DOWNSTAIRS_DESC1;
    scene->text_content_ids[2] = TEXT_EMPTY_LINE;
    scene->text_content_ids[3] = TEXT_DOWNSTAIRS_DESC2;
    scene->text_content_ids[4] = TEXT_EMPTY_LINE;
    scene->text_content_ids[5] = TEXT_DOWNSTAIRS_DESC3;
    scene->text_content_ids[6] = TEXT_EMPTY_LINE;
    scene->text_content_ids[7] = TEXT_DOWNSTAIRS_DESC4;
    scene->text_content_ids[8] = TEXT_EMPTY_LINE;
    scene->text_content_ids[9] = TEXT_DOWNSTAIRS_DESC5;
    scene->text_line_count = 10;

    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_DAD, .action_id = "talk_to_dad"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_MOM, .action_id = "talk_to_mom"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_GET_MILK, .action_id = "get_milk"};
    scene->choices[3] = (StoryChoice){.text_id = TEXT_CHOICE_RETURN_TO_UPSTAIRS, .action_id = "return_to_upstairs"};
    scene->choice_count = 4;
}