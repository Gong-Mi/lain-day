#include "scenes.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "game_types.h"
#include "flag_system.h"
#include <stdlib.h> // For atoi

// All scene init functions are declared here. They are defined in their respective data.c files.
#include "SCENE_00_ENTRY_data.h"
#include "SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE_data.h"
#include "SCENE_01_LAIN_ROOM_data.h"
#include "SCENE_01A_EXAMINE_NAVI_data.h"
#include "SCENE_01B_NAVI_SHUTDOWN_data.h"
#include "SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE_data.h"
#include "SCENE_01D_NAVI_REBOOT_ENDPROLOGUE_data.h"
#include "SCENE_01E_NAVI_CONNECT_ENDPROLOGUE_data.h"
#include "SCENE_02_DOWNSTAIRS_data.h"
#include "SCENE_02B_DAD_REPLY_NO_data.h"
#include "SCENE_02C_DAD_ASK_HELP_data.h"
#include "SCENE_02D_TALK_TO_MOM_NORMAL_data.h"
#include "SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE_data.h"
#include "SCENE_02J_GET_MILK_ENDPROLOGUE_data.h"
#include "SCENE_03_CHAPTER_ONE_INTRO_data.h"
#include "SCENE_04A_TALK_TO_SISTER_COLD_data.h"
#include "SCENE_04B_TALK_TO_SISTER_CURIOUS_data.h"
#include "SCENE_04C_TALK_TO_SISTER_DEFAULT_data.h"
#include "SCENE_EXAMINE_FRIDGE_data.h"
#include "SCENE_IWAKURA_UPPER_HALLWAY_data.h"
#include "SCENE_MIKA_ROOM_LOCKED_data.h"
#include "SCENE_MIKA_ROOM_UNLOCKED_data.h"
#include "SCENE_MIKA_ROOM_EMPTY_data.h"
#include "SCENE_EXAMINE_BOOKSHELF_data.h"
#include "SCENE_SHINJUKU_ABANDONED_SITE_data.h"
#include "SCENE_DAD_HUB_data.h"
#include "SCENE_DAD_DAY_0_data.h"
#include "SCENE_PC_NAVI_DESKTOP_data.h"

// A function pointer type for scene initializers
typedef void (*SceneInitFunc)(StoryScene*);


// The dispatch table mapping scene IDs to their init functions
static const struct {
    const char* id;
    SceneInitFunc func;
} scene_registrations[] = {
    {"SCENE_00_ENTRY", init_scene_scene_00_entry_from_data},
    {"SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE", init_scene_scene_00a_wait_one_minute_endprologue_from_data},
    {"SCENE_01_LAIN_ROOM", init_scene_scene_01_lain_room_from_data},
    {"SCENE_01A_EXAMINE_NAVI", init_scene_scene_01a_examine_navi_from_data},
    {"SCENE_01B_NAVI_SHUTDOWN", init_scene_scene_01b_navi_shutdown_from_data},
    {"SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE", init_scene_scene_01c_talk_to_figure_endprologue_from_data},
    {"SCENE_01D_NAVI_REBOOT_ENDPROLOGUE", init_scene_scene_01d_navi_reboot_endprologue_from_data},
    {"SCENE_01E_NAVI_CONNECT_ENDPROLOGUE", init_scene_scene_01e_navi_connect_endprologue_from_data},
    {"SCENE_02_DOWNSTAIRS", init_scene_scene_02_downstairs_from_data},
    {"SCENE_02B_DAD_REPLY_NO", init_scene_scene_02b_dad_reply_no_from_data},
    {"SCENE_02C_DAD_ASK_HELP", init_scene_scene_02c_dad_ask_help_from_data},
    {"SCENE_02D_TALK_TO_MOM_NORMAL", init_scene_scene_02d_talk_to_mom_normal_from_data},
    {"SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE", init_scene_scene_02g_mom_reply_silent_endprologue_from_data},
    {"SCENE_02J_GET_MILK_ENDPROLOGUE", init_scene_scene_02j_get_milk_endprologue_from_data},
    {"SCENE_03_CHAPTER_ONE_INTRO", init_scene_scene_03_chapter_one_intro_from_data},
    {"SCENE_04A_TALK_TO_SISTER_COLD", init_scene_scene_04a_talk_to_sister_cold_from_data},
    {"SCENE_04B_TALK_TO_SISTER_CURIOUS", init_scene_scene_04b_talk_to_sister_curious_from_data},
    {"SCENE_04C_TALK_TO_SISTER_DEFAULT", init_scene_scene_04c_talk_to_sister_default_from_data},
    {"SCENE_EXAMINE_FRIDGE", init_scene_scene_examine_fridge_from_data},
    {"SCENE_IWAKURA_UPPER_HALLWAY", init_scene_scene_iwakura_upper_hallway_from_data},
    {"SCENE_MIKA_ROOM_LOCKED", init_scene_scene_mika_room_locked_from_data},
    {"SCENE_MIKA_ROOM_UNLOCKED", init_scene_scene_mika_room_unlocked_from_data},
    {"SCENE_MIKA_ROOM_EMPTY", init_scene_scene_mika_room_empty_from_data},
    {"SCENE_EXAMINE_BOOKSHELF", init_scene_scene_examine_bookshelf_from_data},
    {"SCENE_SHINJUKU_ABANDONED_SITE", init_scene_scene_shinjuku_abandoned_site_from_data},
#ifdef CHARACTER_FATHER_ALIVE
    {"SCENE_DAD_HUB", init_scene_scene_dad_hub_from_data},
    {"SCENE_DAD_DAY_0", init_scene_scene_dad_day_0_from_data},
#endif
    {"SCENE_PC_NAVI_DESKTOP", init_scene_scene_pc_navi_desktop_from_data},
};

static const int num_scene_registrations = sizeof(scene_registrations) / sizeof(scene_registrations[0]);

bool transition_to_scene(const char* target_story_file, StoryScene* scene, GameState* game_state) {
#ifdef USE_DEBUG_LOGGING
    const char* scene_id_str = (target_story_file != NULL) ? target_story_file : "NULL";
    fprintf(stderr, "DEBUG: Attempting to transition to scene: %s\n", scene_id_str);
    fprintf(stderr, "DEBUG: transition_to_scene: scene ptr: %p\n", (void*)scene);
#endif

    if (target_story_file == NULL || scene == NULL) return false;
    
    for (int i = 0; i < num_scene_registrations; ++i) {
        if (strcmp(scene_registrations[i].id, target_story_file) == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Scene '%s' found in dispatch table. Initializing...\n", target_story_file);
#endif
            scene_registrations[i].func(scene);
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Successfully initialized scene '%s'.\n", target_story_file);
#endif
            return true;
        }
    }
    
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "ERROR: Scene ID '%s' not found in scene registration table.\n", target_story_file);
#endif
    return false;
}

#include "conditions.h"

bool is_choice_selectable(const StoryChoice* choice, const GameState* game_state) {
    return check_conditions(game_state, choice->conditions, choice->condition_count);
}
