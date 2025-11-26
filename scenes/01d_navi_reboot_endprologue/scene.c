#include "scene.h"
#include "string_ids.h"
#include <string.h>

void init_scene_01d_navi_reboot_endprologue(StoryScene* scene) {
    // Set scene ID and location
    strncpy(scene->scene_id, "01d_navi_reboot_endprologue", MAX_NAME_LENGTH - 1);
    strncpy(scene->location_id, "lain_room", MAX_NAME_LENGTH - 1); // This scene occurs within lain_room

    // Set scene text
    scene->text_content_ids[0] = TEXT_NAVI_REBOOT_DESC1;
    scene->text_content_ids[1] = TEXT_NAVI_REBOOT_DESC2;
    scene->text_content_ids[2] = TEXT_NAVI_REBOOT_YOU_QUOTE1;
    scene->text_content_ids[3] = TEXT_NAVI_REBOOT_DESC3;
    scene->text_content_ids[4] = TEXT_NAVI_REBOOT_GHOST_QUOTE1;
    scene->text_content_ids[5] = TEXT_NAVI_REBOOT_GHOST_QUOTE2;
    scene->text_content_ids[6] = TEXT_NAVI_REBOOT_YOU_QUOTE2;
    scene->text_content_ids[7] = TEXT_NAVI_REBOOT_GHOST_QUOTE3;
    scene->text_content_ids[8] = TEXT_NAVI_REBOOT_YOU_QUOTE3;
    scene->text_content_ids[9] = TEXT_NAVI_REBOOT_DESC4;
    scene->text_content_ids[10] = TEXT_NAVI_REBOOT_GHOST_QUOTE4;
    scene->text_line_count = 11; 

    // Set scene choices
    scene->choices[0].text_id = TEXT_CHOICE_START_CHAPTER_ONE;
    strncpy(scene->choices[0].action_id, "start_chapter_one", MAX_NAME_LENGTH - 1);
    
    scene->choice_count = 1;
}
