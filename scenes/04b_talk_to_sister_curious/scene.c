#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_04b_talk_to_sister_curious(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "04b_talk_to_sister_curious", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text - Fixed parts (4 lines) + Curious quotes (4 lines) = 8 lines
    scene->text_line_count = 8;
    scene->text_content_ids[0] = TEXT_SISTER_FIXED_DESC1;
    scene->text_content_ids[1] = TEXT_SISTER_CURIOUS_QUOTE1; // Conditional part
    scene->text_content_ids[2] = TEXT_SISTER_CURIOUS_QUOTE2; // Conditional part
    scene->text_content_ids[3] = TEXT_SISTER_CURIOUS_QUOTE3; // Conditional part
    scene->text_content_ids[4] = TEXT_SISTER_CURIOUS_QUOTE4; // Conditional part
    scene->text_content_ids[5] = TEXT_SISTER_FIXED_DESC2;
    scene->text_content_ids[6] = TEXT_SISTER_FIXED_DESC3;
    scene->text_content_ids[7] = TEXT_SISTER_FIXED_DESC4;

    // Scene Choices
    scene->choice_count = 3;
    scene->choices[0].text_id = TEXT_CHOICE_READ_EMAIL_FROM_CHISA;
    strncpy(scene->choices[0].action_id, "read_email_from_chisa", MAX_NAME_LENGTH - 1);
    scene->choices[0].condition.flag_name[0] = '\0';

    scene->choices[1].text_id = TEXT_CHOICE_GO_TO_SCHOOL;
    strncpy(scene->choices[1].action_id, "go_to_school", MAX_NAME_LENGTH - 1);
    scene->choices[1].condition.flag_name[0] = '\0';

    scene->choices[2].text_id = TEXT_CHOICE_DELETE_EMAIL_UNREAD;
    strncpy(scene->choices[2].action_id, "delete_email_unread", MAX_NAME_LENGTH - 1);
    scene->choices[2].condition.flag_name[0] = '\0';
}
