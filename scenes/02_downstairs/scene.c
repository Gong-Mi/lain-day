#include "scene.h"
#include "game_types.h" // Required for StoryScene and StoryChoice types
#include "string_ids.h" // Required for StringID enum values
#include <string.h>

void init_scene_02_downstairs(StoryScene* scene) {
    // Clear previous state
    memset(scene, 0, sizeof(StoryScene));

    // Set the location metadata
    strcpy(scene->location_id, "kurani_residence/living_room"); // Corrected: use scene->location_id

    // Text Content - Map directly to pre-defined StringIDs
    scene->text_content_ids[0] = TEXT_INVALID;       // Original title "楼下的客厅与厨房" has no direct ID, using invalid
    scene->text_content_ids[1] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[2] = TEXT_DOWNSTAIRS_DESC1; // "你从楼梯上走下来，一股温暖的、生活的气息扑面而来，与楼上的寂静和黑暗截然不同。"
    scene->text_content_ids[3] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[4] = TEXT_DOWNSTAIRS_DESC2; // "客厅里，爸爸正坐在沙发上，借着落地灯的光看报纸；姐姐则躺在另一边的沙发上，举着手机煲电话粥。"
    scene->text_content_ids[5] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[6] = TEXT_DOWNSTAIRS_DESC3; // "客厅连接着一个开放式厨房，妈妈正在水槽边洗碗，能看到她的背影。冰箱就在她旁边，发出轻微的嗡嗡声。"
    scene->text_content_ids[7] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[8] = TEXT_DOWNSTAIRS_DESC4; // "这个家，看起来无比正常。"
    scene->text_content_ids[9] = TEXT_INVALID;       // Empty line
    scene->text_content_ids[10] = TEXT_DOWNSTAIRS_DESC5; // "你打算怎么做？"
    scene->text_line_count = 11;

    // Choices - Map directly to pre-defined StringIDs
    scene->choices[0] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_DAD, .action_id = "talk_to_dad"};
    scene->choices[1] = (StoryChoice){.text_id = TEXT_CHOICE_TALK_TO_MOM, .action_id = "talk_to_mom"};
    scene->choices[2] = (StoryChoice){.text_id = TEXT_CHOICE_GET_MILK, .action_id = "get_milk"};
    scene->choices[3] = (StoryChoice){.text_id = TEXT_CHOICE_RETURN_TO_UPSTAIRS, .action_id = "return_to_upstairs"};
    scene->choice_count = 4;
}