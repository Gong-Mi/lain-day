#include "executor.h"
#include "scenes.h"
#include "map_loader.h" // Added for get_location_by_id
#include "render_utils.h"
#include "flag_system.h"
#include "cmap.h" // Use the new CMap module
#include "game_types.h" // For struct GameState definition
#include "ecc_time.h"
#include "characters/mika.h"
#include "string_table.h" // For get_string_by_id
#include "systems/embedded_navi.h" // Include the new Embedded NAVI system
#include "systems/navi_mini.h"
#include "systems/navi_pro.h"
#include "systems/navi_alpha.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atoi

// Helper to find an item by its ID
static const Item* find_item_by_id(const char* item_id, const struct GameState* game_state) {
    for (int i = 0; i < game_state->item_count; i++) {
        if (strcmp(game_state->all_items[i].id, item_id) == 0) {
            return &game_state->all_items[i];
        }
    }
    return NULL;
}

// Helper to set flags
static void set_flag(struct GameState* game_state, const char* name, const char* value) {
    hash_table_set(game_state->flags, name, value);
}

// Helper to unlock commands
static void unlock_command(struct GameState* game_state, const char* command) {
    int found = 0;
    for (int i = 0; i < game_state->player_state.unlocked_commands_count; i++) {
        if (strcmp(game_state->player_state.unlocked_commands[i], command) == 0) {
            found = 1;
            break;
        }
    }
    if (!found && game_state->player_state.unlocked_commands_count < MAX_COMMANDS) {
        strncpy(game_state->player_state.unlocked_commands[game_state->player_state.unlocked_commands_count], command, MAX_NAME_LENGTH - 1);
        game_state->player_state.unlocked_commands_count++;
    }
}

// Helper to acquire item
static void acquire_item_logic(struct GameState* game_state, const char* item_id) {
    const Item* item_def = find_item_by_id(item_id, game_state);
    if (item_def != NULL) {
        if (game_state->player_state.credit_level >= item_def->required_credit) {
            int found = 0;
            for (int i = 0; i < game_state->player_state.inventory_count; i++) {
                if (strcmp(game_state->player_state.inventory[i].name, item_def->id) == 0) {
                    game_state->player_state.inventory[i].quantity++;
                    found = 1;
                    break;
                }
            }
            if (!found && game_state->player_state.inventory_count < MAX_INVENTORY_ITEMS) {
                InventoryItem* new_inv_item = &game_state->player_state.inventory[game_state->player_state.inventory_count];
                strncpy(new_inv_item->name, item_def->id, MAX_NAME_LENGTH - 1);
                new_inv_item->quantity = 1;
                game_state->player_state.inventory_count++;
            }
        } else {
            fprintf(stderr, "INFO: Not enough credit to acquire item '%s'. Required: %d, Current: %d\n", item_id, item_def->required_credit, game_state->player_state.credit_level);
        }
    } else {
        fprintf(stderr, "WARNING: Item definition not found for '%s'.\n", item_id);
    }
}


// Helper to get time cost for an action (in minutes)
static int get_action_time_cost(const char* action_id) {
    // --- General Actions ---
    if (strcmp(action_id, "wait_one_minute") == 0) return 1;
    if (strcmp(action_id, "talk_to_dad") == 0) return 5;
    if (strcmp(action_id, "talk_to_mom") == 0) return 5;
    if (strcmp(action_id, "talk_to_sister") == 0) return 5;
    if (strcmp(action_id, "get_milk") == 0) return 3;
    if (strcmp(action_id, "take_milk_from_fridge") == 0) return 1;

    // --- Movement Actions (refer to TIME_COST_DESIGN.md) ---
    // Short-distance
    if (strcmp(action_id, "downstairs") == 0) return 1;
    if (strcmp(action_id, "upstairs") == 0) return 1;
    if (strcmp(action_id, "lains_room") == 0) return 1;
    if (strcmp(action_id, "mikas_room") == 0) return 1;
    if (strcmp(action_id, "bathroom") == 0) return 1;
    if (strcmp(action_id, "study") == 0) return 1;
    if (strcmp(action_id, "living_area") == 0) return 1;
    if (strcmp(action_id, "hallway") == 0) return 1;
    if (strcmp(action_id, "outside") == 0) return 1;
    if (strcmp(action_id, "house") == 0) return 1;
    if (strcmp(action_id, "upper_hallway") == 0) return 1;
    if (strcmp(action_id, "go_to_park") == 0) return 1;
    if (strcmp(action_id, "go_to_center_park") == 0) return 1;
    if (strcmp(action_id, "return_to_street") == 0) return 1;

    // Long-distance (placeholders)
    if (strcmp(action_id, "shibuya") == 0) return 25;
    if (strcmp(action_id, "home") == 0) return 25;
    if (strcmp(action_id, "shinjuku_site") == 0) return 30;
    if (strcmp(action_id, "cyberia") == 0) return 15;
    
    // Default time cost for actions not explicitly listed
    return 0;
}

// Helper to apply time cost
static void apply_time_cost(struct GameState* game_state, const char* action_id) {
    int minutes = get_action_time_cost(action_id);
    if (minutes > 0) {
        const uint32_t time_cost_units = minutes * 60 * 16;
        DecodedTimeResult decoded_result = decode_time_with_ecc(game_state->time_of_day);
        uint32_t new_time = decoded_result.data + time_cost_units;
        
        game_state->time_of_day = encode_time_with_ecc(new_time);
    }
}

// Returns 1 if scene changed, 0 otherwise
int execute_action(const char* action_id, struct GameState* game_state) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: execute_action received action_id: '%s'\n", action_id);
#endif
    if (action_id == NULL || game_state == NULL) {
        return 0;
    }

    int scene_changed = 0;

    // Apply time cost for the action
    apply_time_cost(game_state, action_id);


    // --- Refactored: Generic Connection Handling ---
    const Location* current_loc = (const Location*)cmap_get(game_state->location_map, game_state->player_state.location);
    if (current_loc != NULL) {
        for (int i = 0; i < current_loc->connection_count; i++) {
            const Connection* conn = &current_loc->connections[i];
            if (strcmp(conn->action_id, action_id) == 0) {
                // This action corresponds to a map connection. Check for conditions.
                if (conn->is_accessible != NULL) {
                    if (!conn->is_accessible(game_state, conn)) {
                        // Access is denied.
                        strncpy(game_state->current_story_file, conn->access_denied_scene_id, MAX_PATH_LENGTH - 1);
                        return 1; // Scene changed to "access denied" scene.
                    }
                }
                // If we are here, access is granted.
                mika_return_to_schedule(); // Mika's schedule might change upon player movement
                strncpy(game_state->player_state.location, conn->target_location_id, MAX_NAME_LENGTH - 1);
                if (conn->target_scene_id != NULL) {
                    strncpy(game_state->current_story_file, conn->target_scene_id, MAX_PATH_LENGTH - 1);
                } else {
                    // Fallback: If no specific target scene is provided, try to load a scene based on location ID
                    // This might be a generic "enter location" scene or an empty scene.
                    // For now, we'll just keep the current scene to avoid unexpected transitions.
                    // A better long-term solution would be to define default scenes for locations.
                    fprintf(stderr, "WARNING: Connection to '%s' has no target scene ID. Current scene will persist.\n", conn->target_location_id);
                }
                return 1; // Scene or location has changed.
            }
        }
    }
    
    // --- STORY CHANGE ACTIONS ---
    // New generic scene setter
    if (strncmp(action_id, "SET_SCENE:", 10) == 0) {
        const char* target_scene = action_id + 10;
        strncpy(game_state->current_story_file, target_scene, MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    }
    else if (strcmp(action_id, "lains_room") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01_LAIN_ROOM", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    }
    else if (strcmp(action_id, "use_phone_navi") == 0) {
        enter_embedded_navi(game_state);
        scene_changed = 1;
    } 
    else if (strcmp(action_id, "use_desktop_navi") == 0) {
        enter_navi_mini(game_state);
        scene_changed = 1;
    }
    else if (strcmp(action_id, "use_navi_pro") == 0) {
        enter_navi_pro(game_state);
        scene_changed = 1;
    }
    else if (strcmp(action_id, "use_navi_alpha") == 0) {
        enter_navi_alpha(game_state);
        scene_changed = 1;
    } 
    else if (strcmp(action_id, "go_back_to_shibuya") == 0) {
        mika_return_to_schedule();
        strncpy(game_state->player_state.location, "shibuya_street", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "SCENE_09_CYBERIA", MAX_PATH_LENGTH - 1); // Placeholder scene
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_shinjuku_site") == 0) {
        mika_return_to_schedule();
        strncpy(game_state->player_state.location, "shinjuku_abandoned_site", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "SCENE_SHINJUKU_ABANDONED_SITE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "explore_shinjuku_site") == 0) {
        strncpy(game_state->transient_message, get_string_by_id(TEXT_EXPLORING_SITE_MESSAGE), MAX_LINE_LENGTH - 1);
        game_state->has_transient_message = true;
        scene_changed = 0; // Don't change scene, just show message

        const char* flag_val = hash_table_get(game_state->flags, "door_opened_by_ghost");
        if (flag_val == NULL || strcmp(flag_val, "1") != 0) {
            // Event has not happened yet, trigger it.
            strncpy(game_state->current_story_file, "SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
            set_flag(game_state, "sister_mood", "cold");
            set_flag(game_state, "door_opened_by_ghost", "1"); // Set flag to prevent re-triggering
            scene_changed = 1;
        } else {
            // Event has already happened. Display a transient message.
            strncpy(game_state->transient_message, get_string_by_id(TEXT_WAIT_NOTHING_DESC1), MAX_LINE_LENGTH - 1);
            game_state->has_transient_message = true;
            scene_changed = 0; // Do not change scene, just re-render current scene with message
        }
    } else if (strcmp(action_id, "talk_to_figure") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "cold");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_shutdown") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01B_NAVI_SHUTDOWN", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_reboot") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01D_NAVI_REBOOT_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_connect") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01E_NAVI_CONNECT_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "exit_story") == 0) {
        // This action type typically indicates returning to a previous context,
        // often implied by story flow without explicit current_story_file change.
        // For now, it doesn't cause a scene_changed=1 unless a story_file is explicitly set.
        scene_changed = 0; // Does not change story file by itself
    } else if (strcmp(action_id, "dad_reply_no") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02B_DAD_REPLY_NO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "dad_ask_help") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02C_DAD_ASK_HELP", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "start_chapter_one") == 0) {
        strncpy(game_state->current_story_file, "SCENE_03_CHAPTER_ONE_INTRO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_dad") == 0) {
        strncpy(game_state->current_story_file, "SCENE_DAD_HUB", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "get_milk") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02J_GET_MILK_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_fine") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02F_MOM_REPLY_FINE_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_silent") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "cold");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_deny_vision") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02H_MOM_DENY_VISION_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_agree_doctor") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02I_MOM_AGREE_DOCTOR_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_silent_vision") == 0) {
        strncpy(game_state->current_story_file, "SCENE_02K_MOM_SILENT_VISION_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_cold") == 0) {
        strncpy(game_state->current_story_file, "SCENE_04A_TALK_TO_SISTER_COLD", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_curious") == 0) {
        strncpy(game_state->current_story_file, "SCENE_04B_TALK_TO_SISTER_CURIOUS", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_default") == 0) {
        strncpy(game_state->current_story_file, "SCENE_04C_TALK_TO_SISTER_DEFAULT", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_shutdown_story") == 0) {
        strncpy(game_state->current_story_file, "SCENE_01B_NAVI_SHUTDOWN", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "read_email_from_chisa") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_EMAIL_CLIENT", MAX_PATH_LENGTH - 1);
        unlock_command(game_state, "mail");
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_school") == 0) {
        strncpy(game_state->current_story_file, "SCENE_06_TRAIN_SCENE", MAX_PATH_LENGTH - 1);
        // Assuming other commands are unlocked by default now or elsewhere
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_classroom") == 0) {
        strncpy(game_state->current_story_file, "SCENE_07_CLASSROOM", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_teacher_knows") == 0) {
        strncpy(game_state->current_story_file, "SCENE_08B_ASK_TEACHER", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_teacher", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_about_proxy") == 0) {
        strncpy(game_state->current_story_file, "SCENE_08C_ASK_PROXY", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_proxy", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_about_chisa") == 0) {
        strncpy(game_state->current_story_file, "SCENE_08D_ASK_CHISA", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_chisa", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_bar") == 0) {
        strncpy(game_state->current_story_file, "SCENE_09_CYBERIA", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "persuade_to_bar") == 0) {
        strncpy(game_state->current_story_file, "SCENE_09A_PERSUASION", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_alice_scared") == 0) {
        strncpy(game_state->current_story_file, "SCENE_08E_ASK_ALICE_SCARED", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "active_overload") == 0) {
        strncpy(game_state->current_story_file, "SCENE_06Z_TRAIN_EVENT_RESULT", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "overload_result", "active");
        scene_changed = 1;
    } else if (strcmp(action_id, "passive_overload") == 0) {
        strncpy(game_state->current_story_file, "SCENE_06Z_TRAIN_EVENT_RESULT", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "overload_result", "passive");
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_ch2_cold_open") == 0) {
        strncpy(game_state->current_story_file, "SCENE_19_COLD_OPEN_CH2", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "start_chapter_two") == 0) {
        strncpy(game_state->current_story_file, "SCENE_20_CHAPTER_TWO_INTRO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_hug_alice") == 0) {
        strncpy(game_state->current_story_file, "SCENE_21A_HUG_ALICE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_reply_nothing") == 0) {
        strncpy(game_state->current_story_file, "SCENE_21B_REPLY_FINE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_ask_who") == 0) {
        strncpy(game_state->current_story_file, "SCENE_CH2_ASK_WHO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_hug_alice_continue") == 0) {
        strncpy(game_state->current_story_file, "SCENE_21C_ALICE_COMFORTS_LAIN", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_bar_music_interrupt") == 0) {
        strncpy(game_state->current_story_file, "SCENE_22_CYBERIA_FLASHBACK", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "reply_is_me") == 0) {
        strncpy(game_state->current_story_file, "SCENE_22D_REPLY_IS_ME", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "boss_invites_lain_to_sing") == 0) {
        strncpy(game_state->current_story_file, "SCENE_22B_BOSS_INVITES_LAIN_TO_SING", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "step_on_stage") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_OLD_MIC", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_echo") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_SINGING_RESULT_ECHO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot_stare") == 0) {
        strncpy(game_state->current_story_file, "SCENE_GUNSHOT_ADVANCE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_plastic_love") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_SINGING_RESULT_ECHO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_op") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_SINGING_RESULT_ECHO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_ed") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_SINGING_RESULT_ECHO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SINGING_RESULT_GUNSHOT", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "examine_old_mic") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_OLD_MIC", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot_advance") == 0) {
        strncpy(game_state->current_story_file, "SCENE_GUNSHOT_ADVANCE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
        } else if (strcmp(action_id, "gunshot_exit") == 0) {
            strncpy(game_state->current_story_file, "SCENE_00_ENTRY", MAX_PATH_LENGTH - 1);
            const uint32_t default_start_time_units = 8 * 60 * 60 * 16;
            game_state->time_of_day = encode_time_with_ecc(default_start_time_units);
            hash_table_set(game_state->flags, "TIME_GLITCH_ACTIVE", "0");
            scene_changed = 1;
        } else if (strcmp(action_id, "end_chapter_two") == 0) {
        strncpy(game_state->current_story_file, "SCENE_CHAPTER_THREE_INTRO", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    }

    // --- ACQUIRE ITEM ACTIONS ---
    else if (strcmp(action_id, "order_milk") == 0) {
        acquire_item_logic(game_state, "milk");
    } else if (strcmp(action_id, "order_coffee") == 0) {
        acquire_item_logic(game_state, "coffee");
    } else if (strcmp(action_id, "order_juice") == 0) {
        acquire_item_logic(game_state, "juice");
    } else if (strcmp(action_id, "acquire_alice_hat") == 0) {
        acquire_item_logic(game_state, "alice_hat");
    } else if (strcmp(action_id, "take_sand_bottle") == 0) {
        acquire_item_logic(game_state, "sand_bottle");
        set_flag(game_state, "sand_bottle_taken", "true");
    } else if (strcmp(action_id, "take_milk_from_fridge") == 0) {
        acquire_item_logic(game_state, "milk");

        // Re-render the same scene, in case we want to make the choice conditional later
        strncpy(game_state->current_story_file, "SCENE_EXAMINE_FRIDGE", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    }
    
    // --- TOGGLE PROTOCOL ACTIONS ---
    else if (strcmp(action_id, "toggle_ipv4") == 0) {
        const char* current_status = hash_table_get(game_state->flags, "network_status.protocols.ipv4");
        if (current_status == NULL || strcmp(current_status, "off") == 0) {
            set_flag(game_state, "network_status.protocols.ipv4", "on");
        } else {
            set_flag(game_state, "network_status.protocols.ipv4", "off");
        }
    } else if (strcmp(action_id, "toggle_ipv6") == 0) {
        const char* current_status = hash_table_get(game_state->flags, "network_status.protocols.ipv6");
        if (current_status == NULL || strcmp(current_status, "off") == 0) {
            set_flag(game_state, "network_status.protocols.ipv6", "on");
        } else {
            set_flag(game_state, "network_status.protocols.ipv6", "off");
        }
    } else if (strcmp(action_id, "toggle_ip7") == 0) {
        const char* current_status = hash_table_get(game_state->flags, "network_status.protocols.ip7");
        if (current_status == NULL || strcmp(current_status, "off") == 0) {
            set_flag(game_state, "network_status.protocols.ip7", "on");
        } else {
            set_flag(game_state, "network_status.protocols.ip7", "off");
        }
    }


    // --- CONDITIONAL ACTIONS ---
    // The logic for talking to the sister is now encapsulated in the Mika module.
    else if (strcmp(action_id, "talk_to_sister") == 0) {
        get_mika_module()->on_talk(game_state);
        // Note: The on_talk function itself calls execute_action internally,
        // which will set scene_changed. We assume we don't need to capture the return
        // value here, as the recursive call will handle the scene change.
        // This is a slight awkwardness of the current hybrid design.
        scene_changed = 1; // Mark scene as changed to ensure re-rendering.
    }
    // Example of conditional_story_change (simplified for enter_chatroom)
    else if (strcmp(action_id, "enter_chatroom") == 0) {
        const char* chat_url = hash_table_get(game_state->flags, "active_chat_url");
        if (chat_url != NULL && strlen(chat_url) > 0) { // Assuming active_chat_url means real chat
            strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_CHATROOM_REAL", MAX_PATH_LENGTH - 1);
        } else {
            strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_CHATROOM_EMPTY", MAX_PATH_LENGTH - 1);
        }
        scene_changed = 1;
    }
    
    // --- GENERIC FLAG SETTING ACTIONS (for dynamic values like typewriter_delay, network scope) ---
    else if (strcmp(action_id, "set_font_speed_fast") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.02");
        scene_changed = 1;
    } else if (strcmp(action_id, "set_font_speed_normal") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.04");
        scene_changed = 1;
    } else if (strcmp(action_id, "set_font_speed_slow") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.07");
        scene_changed = 1;
    } else if (strcmp(action_id, "connect_to_regional") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_NETWORK_STATUS", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "network_status.scope", "地区局域网");
        scene_changed = 1;
    } else if (strcmp(action_id, "connect_to_national") == 0) {
        strncpy(game_state->current_story_file, "SCENE_SIDE_STORIES_NETWORK_STATUS", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "network_status.scope", "全国互联网");
        scene_changed = 1;
    }


    // --- UNRECOGNIZED ACTION ---
    else {
        fprintf(stderr, "WARNING: Unrecognized action ID: %s\n", action_id);
    }
    
    // --- TIME PROGRESSION (simplified, as time_cost was dynamic per action) ---
    // For now, let's assume a default time cost or handle it within specific actions.
    // game_state->time_of_day += DEFAULT_TIME_COST; // Or implement per action
    // Handle wrapping around midnight if needed in specific actions.

    // Always update player's location if it was changed by action
    // The location change logic is moved into respective action blocks above.

    return scene_changed;
}

bool execute_command(const char* input, GameState* game_state) {
    if (input == NULL || game_state == NULL) {
        return false; // No re-render for invalid input
    }

    // Command: inventory / inv
    if (strcmp(input, "inventory") == 0 || strcmp(input, "inv") == 0) {
        printf("\n--- Inventory ---\n");
        if (game_state->player_state.inventory_count == 0) {
            printf("  (empty)\n");
        }
        for (int i = 0; i < game_state->player_state.inventory_count; i++) {
            printf("  - %s: %d\n", game_state->player_state.inventory[i].name, game_state->player_state.inventory[i].quantity);
        }
        printf("-----------------\n");
        return false; // No re-render needed for inventory
    }
    // Command: arls / arls <poi_id>
    else if (strncmp(input, "arls", 4) == 0) {
        char poi_id_buffer[MAX_NAME_LENGTH];
        int scan_result = sscanf(input, "arls %s", poi_id_buffer);
        if (scan_result == 1) { // Command is "arls <something>"
            const Location* current_loc = (const Location*)cmap_get(game_state->location_map, game_state->player_state.location);
            if (current_loc) {
                for (int i = 0; i < current_loc->pois_count; i++) {
                    if (strcmp(current_loc->pois[i].id, poi_id_buffer) == 0) {
#ifdef USE_DEBUG_LOGGING
                        fprintf(stderr, "DEBUG: Arls: Found POI '%s'. view_scene_id: '%s'\n", 
                                current_loc->pois[i].id, 
                                current_loc->pois[i].view_scene_id ? current_loc->pois[i].view_scene_id : "NULL");
#endif
                        if (current_loc->pois[i].view_scene_id != NULL) {
                            strncpy(game_state->current_story_file, current_loc->pois[i].view_scene_id, MAX_PATH_LENGTH - 1);
                            return true; // Re-render needed for scene change
                        } else {
                            printf("You examine the %s: %s\n", current_loc->pois[i].name, current_loc->pois[i].description);
                            return false;
                        }
                    }
                }
            }
            printf("'%s' is not a valid point of interest here.\n", poi_id_buffer);
            return false;
        } else { // Command is just "arls" (no specific POI ID)
            printf("\n--- Area List Scan ---\n");
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "Executor ERROR: Arls: Player location ID is '%s'.\n", game_state->player_state.location);
#endif
            Location *current_loc = get_location_by_id(game_state->player_state.location);
            if (current_loc) {
#ifdef USE_DEBUG_LOGGING
                fprintf(stderr, "DEBUG: Arls: Retrieved location '%s', pois_count: %d. Description (first 50 chars): '%.50s...'\n", current_loc->id, current_loc->pois_count, current_loc->description);
#endif
                render_scene_description(current_loc->description);

                if (current_loc->pois_count > 0) {
                    render_text("\n\n Points of Interest: \n\n");
                    for (int i = 0; i < current_loc->pois_count; i++) {
#ifdef USE_DEBUG_LOGGING
                        fprintf(stderr, "DEBUG: Arls: Printing POI '%s'.\n", current_loc->pois[i].name);
#endif
                        render_poi_name(current_loc->pois[i].name);
                        render_text("\n");
                    }
                }

                if (current_loc->connection_count > 0) {
                    render_text("\n\nConnections:\n\n");
                    for (int i = 0; i < current_loc->connection_count; i++) {
                        char conn_buf[MAX_LINE_LENGTH];
                        const Connection* conn = &current_loc->connections[i];
                        snprintf(conn_buf, MAX_LINE_LENGTH, "  - %s -> %s\n", conn->action_id, conn->target_location_id);
                        render_text(conn_buf);
                    }
                }
            }
#ifdef USE_DEBUG_LOGGING
            else {
                fprintf(stderr, "ERROR: Arls: get_location_by_id returned NULL for ID '%s'. Current location '%s' not found in map data.\n", game_state->player_state.location, game_state->player_state.location);
            }
#endif
            printf("----------------------\n");
            return false;
        }
    }
    // Command: navi
    else if (strcmp(input, "navi") == 0) {
        enter_embedded_navi(game_state);
        return true; // Re-render needed after exiting NAVI
    }
    // Command: exper <poi_id>
    else if (strncmp(input, "exper ", 6) == 0) {
        char poi_id_buffer[MAX_NAME_LENGTH] = {0};
        int scan_result = sscanf(input, "exper %s", poi_id_buffer);

        if (scan_result == 1) {
            const Location* current_loc = (const Location*)cmap_get(game_state->location_map, game_state->player_state.location);
            if (current_loc) {
                for (int i = 0; i < current_loc->pois_count; i++) {
                    if (strcmp(current_loc->pois[i].id, poi_id_buffer) == 0) {
                                                    if (current_loc->pois[i].examine_action_id != NULL) {
                        #ifdef USE_DEBUG_LOGGING
                                                        fprintf(stderr, "DEBUG: Calling execute_action with examine_action_id: '%s'\n", current_loc->pois[i].examine_action_id);
                        #endif
                                                        return execute_action(current_loc->pois[i].examine_action_id, game_state);
                                                    } else {                            printf("You can't use or interact with the %s in that way.\n", current_loc->pois[i].name);
                            return false;
                        }
                    }
                }
            }
            printf("'%s' is not a valid point of interest here.\n", poi_id_buffer);
            return false;
        } else {
            printf("Usage: exper <object_id>\n");
            return false;
        }
    }
    // Command: move <destination>
    else if (strncmp(input, "move ", 5) == 0) {
        char destination_buffer[MAX_NAME_LENGTH] = {0};
        sscanf(input, "move %s", destination_buffer);

        if (strlen(destination_buffer) > 0) {
            const Location* current_loc = (const Location*)cmap_get(game_state->location_map, game_state->player_state.location);
            if (current_loc) {
                for (int i = 0; i < current_loc->connection_count; i++) {
                    if (strcmp(current_loc->connections[i].action_id, destination_buffer) == 0) {
                        return execute_action(destination_buffer, game_state);
                    }
                }
            }
            printf("You can't move to '%s' from here.\n", destination_buffer);
            return false;
        } else {
            printf("Usage: move <destination>\n");
            return false;
        }
    }
    // Command: help
    else if (strcmp(input, "help") == 0) {
        printf("\n--- Help ---\n");
        printf("Available commands:\n");
        for (int i = 0; i < game_state->player_state.unlocked_commands_count; i++) {
            printf("  - %s\n", game_state->player_state.unlocked_commands[i]);
        }
        printf("  - quit\n");
        printf("-------------\n");
        return false; // No re-render needed for help
    }
    // Command: time
    else if (strcmp(input, "time") == 0) {
        printf("\n--- Time ---\n");
        print_game_time(game_state->time_of_day);
        printf("-----------\n");
        return false; // No re-render needed for time
    }
    // Command: debug_time (Hidden)
    else if (strncmp(input, "debug_time", 10) == 0) {
        int minutes_to_add = 0;
        if (sscanf(input, "debug_time add %d", &minutes_to_add) == 1) {
            if (minutes_to_add > 0) {
                DecodedTimeResult res = decode_time_with_ecc(game_state->time_of_day);
                // 16 units per second * 60 seconds = 960 units per minute
                uint32_t units_to_add = (uint32_t)minutes_to_add * 60 * 16;
                uint32_t new_total = res.data + units_to_add;
                game_state->time_of_day = encode_time_with_ecc(new_total);
                printf("Debug: Added %d minutes.\n", minutes_to_add);
            } else {
                printf("Debug: Please specify a positive number of minutes.\n");
            }
        } else {
            DecodedTimeResult res = decode_time_with_ecc(game_state->time_of_day);
            uint32_t total_minutes = res.data / (60 * 16);
            printf("\n--- Debug Time ---\n");
            printf("Raw Units: %u\n", res.data);
            printf("Total Accumulated Minutes: %u\n", total_minutes);
            printf("Day Cycle Position: %02d:%02d\n", (total_minutes / 60) % 24, total_minutes % 60);
            printf("------------------\n");
        }
        return false;
    }
    // Unrecognized command
    else {
        printf("Command not recognized: %s\n", input);
        return false; // No re-render needed for unrecognized command
    }
}