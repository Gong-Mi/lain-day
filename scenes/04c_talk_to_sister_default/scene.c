#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_04c_talk_to_sister_default(StoryScene* scene) {
    // Scene ID
    strncpy(scene->scene_id, "04c_talk_to_sister_default", MAX_NAME_LENGTH - 1);

    // Location ID
    strncpy(scene->location_id, "kurani_residence/living_room", MAX_NAME_LENGTH - 1);

    // Scene Text - Fixed parts (4 lines) + Default quotes (4 lines) = 8 lines
    scene->dialogue_line_count = 8;
    scene->dialogue_lines[0] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_SISTER_FIXED_DESC1};
    scene->dialogue_lines[1] = (DialogueLine){.speaker_id = SPEAKER_MIRA, .text_id = TEXT_SISTER_DEFAULT_QUOTE1};
    scene->dialogue_lines[2] = (DialogueLine){.speaker_id = SPEAKER_MIRA, .text_id = TEXT_SISTER_DEFAULT_QUOTE2};
    scene->dialogue_lines[3] = (DialogueLine){.speaker_id = SPEAKER_LAIN, .text_id = TEXT_SISTER_DEFAULT_YOU_QUOTE1};
    scene->dialogue_lines[4] = (DialogueLine){.speaker_id = SPEAKER_MIRA, .text_id = TEXT_SISTER_DEFAULT_QUOTE3};
    scene->dialogue_lines[5] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_SISTER_FIXED_DESC2};
    scene->dialogue_lines[6] = (DialogueLine){.speaker_id = SPEAKER_MIRA, .text_id = TEXT_SISTER_FIXED_DESC3};
    scene->dialogue_lines[7] = (DialogueLine){.speaker_id = SPEAKER_NONE, .text_id = TEXT_SISTER_FIXED_DESC4};

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
