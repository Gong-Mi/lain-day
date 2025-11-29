#include "executor.h"
#include "flag_system.h"
#include "cmap.h" // Use the new CMap module
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atoi

// Helper to find an item by its ID
static const Item* find_item_by_id(const char* item_id, const GameState* game_state) {
    for (int i = 0; i < game_state->item_count; i++) {
        if (strcmp(game_state->all_items[i].id, item_id) == 0) {
            return &game_state->all_items[i];
        }
    }
    return NULL;
}

// Helper to set flags
static void set_flag(GameState* game_state, const char* name, const char* value) {
    hash_table_set(game_state->flags, name, value);
}

// Helper to unlock commands
static void unlock_command(GameState* game_state, const char* command) {
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
static void acquire_item_logic(GameState* game_state, const char* item_id) {
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


// Returns 1 if scene changed, 0 otherwise
int execute_action(const char* action_id, GameState* game_state) {
    if (action_id == NULL || game_state == NULL) {
        return 0;
    }

    int scene_changed = 0;
    
    // --- LOCATION CHANGE ACTIONS ---
    if (strcmp(action_id, "enter_lain_room") == 0) {
        strncpy(game_state->player_state.location, "lain_room", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "story/01_lain_room.md", MAX_PATH_LENGTH - 1);
        game_state->time_of_day += 10; // Example time cost
        scene_changed = 1;
    } else if (strcmp(action_id, "go_downstairs") == 0) {
        strncpy(game_state->player_state.location, "downstairs", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "story/02_downstairs.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "return_to_entry") == 0) {
        strncpy(game_state->player_state.location, "entry", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "story/00_entry.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "return_to_upstairs") == 0) { // Example for returning from dynamic maps
        strncpy(game_state->player_state.location, "lain_room", MAX_NAME_LENGTH - 1);
        strncpy(game_state->current_story_file, "story/01_lain_room.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "return_to_living_room") == 0) {
        strncpy(game_state->player_state.location, "kurani_residence/living_room", MAX_NAME_LENGTH - 1); // This location needs to exist programmatically
        strncpy(game_state->current_story_file, "story/02_downstairs.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    }


    // --- STORY CHANGE ACTIONS ---
    else if (strcmp(action_id, "examine_navi") == 0) {
        strncpy(game_state->current_story_file, "story/01a_examine_navi.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "wait_one_minute") == 0) {
        strncpy(game_state->current_story_file, "story/00a_wait_one_minute_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "cold");
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_figure") == 0) {
        strncpy(game_state->current_story_file, "story/01c_talk_to_figure_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "cold");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_shutdown") == 0) {
        strncpy(game_state->current_story_file, "story/01b_navi_shutdown.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_reboot") == 0) {
        strncpy(game_state->current_story_file, "story/01d_navi_reboot_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "navi_connect") == 0) {
        strncpy(game_state->current_story_file, "story/01e_navi_connect_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "curious");
        scene_changed = 1;
    } else if (strcmp(action_id, "exit_story") == 0) {
        // This action type typically indicates returning to a previous context,
        // often implied by story flow without explicit current_story_file change.
        // For now, it doesn't cause a scene_changed=1 unless a story_file is explicitly set.
        scene_changed = 0; // Does not change story file by itself
    } else if (strcmp(action_id, "dad_reply_no") == 0) {
        strncpy(game_state->current_story_file, "story/02b_dad_reply_no.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "dad_ask_help") == 0) {
        strncpy(game_state->current_story_file, "story/02c_dad_ask_help.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "start_chapter_one") == 0) {
        strncpy(game_state->current_story_file, "story/03_chapter_one_intro.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_dad") == 0) {
        strncpy(game_state->current_story_file, "story/02a_talk_to_dad.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_mom") == 0) {
        strncpy(game_state->current_story_file, "story/02d_talk_to_mom_normal.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "get_milk") == 0) {
        strncpy(game_state->current_story_file, "story/02j_get_milk_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_fine") == 0) {
        strncpy(game_state->current_story_file, "story/02f_mom_reply_fine_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_silent") == 0) {
        strncpy(game_state->current_story_file, "story/02g_mom_reply_silent_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "cold");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_deny_vision") == 0) {
        strncpy(game_state->current_story_file, "story/02h_mom_deny_vision_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_agree_doctor") == 0) {
        strncpy(game_state->current_story_file, "story/02i_mom_agree_doctor_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "mom_reply_silent_vision") == 0) {
        strncpy(game_state->current_story_file, "story/02k_mom_silent_vision_endprologue.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "sister_mood", "normal");
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_cold") == 0) {
        strncpy(game_state->current_story_file, "story/04a_talk_to_sister_cold.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_curious") == 0) {
        strncpy(game_state->current_story_file, "story/04b_talk_to_sister_curious.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "talk_to_sister_default") == 0) {
        strncpy(game_state->current_story_file, "story/04c_talk_to_sister_default.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_shutdown_story") == 0) {
        strncpy(game_state->current_story_file, "story/01b_navi_shutdown.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "read_email_from_chisa") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/email_client.md", MAX_PATH_LENGTH - 1);
        unlock_command(game_state, "mail");
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_school") == 0) {
        strncpy(game_state->current_story_file, "story/06_train_scene.md", MAX_PATH_LENGTH - 1);
        // Assuming other commands are unlocked by default now or elsewhere
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_classroom") == 0) {
        strncpy(game_state->current_story_file, "story/07_classroom.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_teacher_knows") == 0) {
        strncpy(game_state->current_story_file, "story/08b_ask_teacher.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_teacher", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_about_proxy") == 0) {
        strncpy(game_state->current_story_file, "story/08c_ask_proxy.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_proxy", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_about_chisa") == 0) {
        strncpy(game_state->current_story_file, "story/08d_ask_chisa.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "asked_chisa", "1");
        scene_changed = 1;
    } else if (strcmp(action_id, "go_to_bar") == 0) {
        strncpy(game_state->current_story_file, "story/09_cyberia.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "persuade_to_bar") == 0) {
        strncpy(game_state->current_story_file, "story/09a_persuasion.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ask_alice_scared") == 0) {
        strncpy(game_state->current_story_file, "story/08e_ask_alice_scared.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "active_overload") == 0) {
        strncpy(game_state->current_story_file, "story/06z_train_event_result.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "overload_result", "active");
        scene_changed = 1;
    } else if (strcmp(action_id, "passive_overload") == 0) {
        strncpy(game_state->current_story_file, "story/06z_train_event_result.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "overload_result", "passive");
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_ch2_cold_open") == 0) {
        strncpy(game_state->current_story_file, "story/19_cold_open_ch2.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "start_chapter_two") == 0) {
        strncpy(game_state->current_story_file, "story/20_chapter_two_intro.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_hug_alice") == 0) {
        strncpy(game_state->current_story_file, "story/21a_hug_alice.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_reply_nothing") == 0) {
        strncpy(game_state->current_story_file, "story/21b_reply_fine.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_ask_who") == 0) {
        strncpy(game_state->current_story_file, "story/ch2_ask_who.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_hug_alice_continue") == 0) {
        strncpy(game_state->current_story_file, "story/21c_alice_comforts_lain.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "ch2_bar_music_interrupt") == 0) {
        strncpy(game_state->current_story_file, "story/22_cyberia_flashback.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "reply_is_me") == 0) {
        strncpy(game_state->current_story_file, "story/22d_reply_is_me.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "boss_invites_lain_to_sing") == 0) {
        strncpy(game_state->current_story_file, "story/22b_boss_invites_lain_to_sing.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "step_on_stage") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/old_mic.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_gunshot") == 0) {
        strncpy(game_state->current_story_file, "story/singing_result_gunshot.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "trigger_echo") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/singing_result_echo.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot_stare") == 0) {
        strncpy(game_state->current_story_file, "story/gunshot_advance.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_plastic_love") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/singing_result_echo.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_op") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/singing_result_echo.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "sing_ed") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/singing_result_echo.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot") == 0) {
        strncpy(game_state->current_story_file, "story/singing_result_gunshot.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "examine_old_mic") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/old_mic.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot_advance") == 0) {
        strncpy(game_state->current_story_file, "story/gunshot_advance.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "gunshot_exit") == 0) {
        strncpy(game_state->current_story_file, "story/singing_result_gunshot.md", MAX_PATH_LENGTH - 1);
        scene_changed = 1;
    } else if (strcmp(action_id, "end_chapter_two") == 0) {
        strncpy(game_state->current_story_file, "story/chapter_three_intro.md", MAX_PATH_LENGTH - 1);
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
    // Example of conditional_action_by_flag (simplified)
    else if (strcmp(action_id, "talk_to_sister") == 0) {
        const char* sister_mood = hash_table_get(game_state->flags, "sister_mood");
        if (sister_mood != NULL) {
            if (strcmp(sister_mood, "cold") == 0) {
                scene_changed = execute_action("talk_to_sister_cold", game_state);
            } else if (strcmp(sister_mood, "curious") == 0) {
                scene_changed = execute_action("talk_to_sister_curious", game_state);
            } else { // default
                scene_changed = execute_action("talk_to_sister_default", game_state);
            }
        } else { // If flag not set, use default
            scene_changed = execute_action("talk_to_sister_default", game_state);
        }
    }
    // Example of conditional_story_change (simplified for enter_chatroom)
    else if (strcmp(action_id, "enter_chatroom") == 0) {
        const char* chat_url = hash_table_get(game_state->flags, "active_chat_url");
        if (chat_url != NULL && strlen(chat_url) > 0) { // Assuming active_chat_url means real chat
            strncpy(game_state->current_story_file, "story/side_stories/chatroom_real.md", MAX_PATH_LENGTH - 1);
        } else {
            strncpy(game_state->current_story_file, "story/side_stories/chatroom_empty.md", MAX_PATH_LENGTH - 1);
        }
        scene_changed = 1;
    }
    
    // --- GENERIC FLAG SETTING ACTIONS (for dynamic values like typewriter_delay, network scope) ---
    else if (strcmp(action_id, "set_font_speed_fast") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/adjust_font_interval.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.02");
        scene_changed = 1;
    } else if (strcmp(action_id, "set_font_speed_normal") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/adjust_font_interval.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.04");
        scene_changed = 1;
    } else if (strcmp(action_id, "set_font_speed_slow") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/adjust_font_interval.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "typewriter_delay", "0.07");
        scene_changed = 1;
    } else if (strcmp(action_id, "connect_to_regional") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/network_status.md", MAX_PATH_LENGTH - 1);
        set_flag(game_state, "network_status.scope", "地区局域网");
        scene_changed = 1;
    } else if (strcmp(action_id, "connect_to_national") == 0) {
        strncpy(game_state->current_story_file, "story/side_stories/network_status.md", MAX_PATH_LENGTH - 1);
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

void execute_command(const char* input, GameState* game_state) {
    if (input == NULL || game_state == NULL) {
        return;
    }

    if (strcmp(input, "inventory") == 0 || strcmp(input, "inv") == 0) {
        printf("\n--- Inventory ---\n");
        if (game_state->player_state.inventory_count == 0) {
            printf("  (empty)\n");
        }
        for (int i = 0; i < game_state->player_state.inventory_count; i++) {
            printf("  - %s: %d\n", game_state->player_state.inventory[i].name, game_state->player_state.inventory[i].quantity);
        }
        printf("-----------------\n");
    }
    else if (strcmp(input, "arls") == 0) {
        printf("\n--- Area List Scan ---\n");
        // Refactored to use the CMap hash table for O(1) average lookup time
        const Location* current_loc = (const Location*)cmap_get(game_state->location_map, game_state->player_state.location);
        if (current_loc) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Arls: Retrieved location '%s', pois_count: %d\n", current_loc->id, current_loc->pois_count);
#endif
            print_colored_line(current_loc->name, (const struct GameState*)game_state);
            print_colored_line(current_loc->description, (const struct GameState*)game_state);
            printf("\nPoints of Interest:\n");
            if (current_loc->pois_count == 0) {
                print_colored_line("  (none)", (const struct GameState*)game_state);
            }
            for (int i = 0; i < current_loc->pois_count; i++) {
#ifdef USE_DEBUG_LOGGING
                fprintf(stderr, "DEBUG: Arls: Printing POI '%s'\n", current_loc->pois[i].name);
#endif
                char poi_buf[MAX_LINE_LENGTH];
                snprintf(poi_buf, MAX_LINE_LENGTH, "  - %s", current_loc->pois[i].name);
                print_colored_line(poi_buf, (const struct GameState*)game_state);
            }
            printf("\nConnections:\n");
            if (current_loc->connection_count == 0) {
                print_colored_line("  (none)", (const struct GameState*)game_state);
            }
            for (int i = 0; i < current_loc->connection_count; i++) {
                char conn_buf[MAX_LINE_LENGTH];
                snprintf(conn_buf, MAX_LINE_LENGTH, "  - %s", current_loc->connections[i]);
                print_colored_line(conn_buf, (const struct GameState*)game_state);
            }
        } else {
            printf("Error: Current location '%s' not found in map data.\n", game_state->player_state.location);
        }
        printf("----------------------\n");
    }
    else if (strcmp(input, "help") == 0) {
        printf("\n--- Help ---\n");
        printf("Available commands:\n");
        for (int i = 0; i < game_state->player_state.unlocked_commands_count; i++) {
            printf("  - %s\n", game_state->player_state.unlocked_commands[i]);
        }
        printf("  - quit\n");
        printf("-------------\n");
    }
    else {
        printf("Command not recognized: %s\n", input);
    }
}