#include "executor.h"
#include "flag_system.h"
#include "cmap.h" // Use the new CMap module
#include <stdio.h>
#include <string.h>

// Helper to find an action by its ID
static const Action* find_action_by_id(const char* action_id, const GameState* game_state) {
    for (int i = 0; i < game_state->action_count; i++) {
        if (strcmp(game_state->all_actions[i].id, action_id) == 0) {
            return &game_state->all_actions[i];
        }
    }
    return NULL;
}

// Helper to find an item by its ID
static const Item* find_item_by_id(const char* item_id, const GameState* game_state) {
    for (int i = 0; i < game_state->item_count; i++) {
        if (strcmp(game_state->all_items[i].id, item_id) == 0) {
            return &game_state->all_items[i];
        }
    }
    return NULL;
}

// Returns 1 if scene changed, 0 otherwise
int execute_action(const char* action_id, GameState* game_state) {
    if (action_id == NULL || game_state == NULL) {
        return 0;
    }

    const Action* action = find_action_by_id(action_id, game_state);
    if (action == NULL) {
        return 0;
    }

    const cJSON *payload = action->payload_json;
    if (payload == NULL) {
        return 0;
    }

    int scene_changed = 0;
    
    // --- STORY/LOCATION CHANGE ---
    // This block handles multiple action types that can result in a scene change.
    if (strcmp(action->type_str, "conditional_story_change") == 0) {
        const cJSON *flag_name_json = cJSON_GetObjectItemCaseSensitive(payload, "flag_name");
        if (cJSON_IsString(flag_name_json) && flag_name_json->valuestring != NULL) {
            const char* flag_value = hash_table_get(game_state->flags, flag_name_json->valuestring);
            
            // Consider the condition true if the flag exists and its value is not "false", "0", or empty.
            int condition_is_true = (flag_value != NULL && strcmp(flag_value, "false") != 0 && strcmp(flag_value, "0") != 0 && strlen(flag_value) > 0);

            if (condition_is_true) {
                const cJSON *story_if_true_json = cJSON_GetObjectItemCaseSensitive(payload, "story_if_true");
                if (cJSON_IsString(story_if_true_json) && story_if_true_json->valuestring != NULL) {
                    strncpy(game_state->current_story_file, story_if_true_json->valuestring, MAX_PATH_LENGTH - 1);
                    scene_changed = 1;
                }
            } else {
                const cJSON *story_if_false_json = cJSON_GetObjectItemCaseSensitive(payload, "story_if_false");
                if (cJSON_IsString(story_if_false_json) && story_if_false_json->valuestring != NULL) {
                    strncpy(game_state->current_story_file, story_if_false_json->valuestring, MAX_PATH_LENGTH - 1);
                    scene_changed = 1;
                }
            }
        }
    } else if (strcmp(action->type_str, "conditional_action_by_flag") == 0) {
        const cJSON *flag_name_json = cJSON_GetObjectItemCaseSensitive(payload, "flag_name");
        if (cJSON_IsString(flag_name_json) && flag_name_json->valuestring != NULL) {
            const char* flag_value = hash_table_get(game_state->flags, flag_name_json->valuestring);
            
            const char* next_action_id = NULL;
            if (flag_value != NULL) {
                if (strcmp(flag_value, "cold") == 0) {
                    const cJSON *cold_action_json = cJSON_GetObjectItemCaseSensitive(payload, "value_cold_action");
                    if (cJSON_IsString(cold_action_json)) next_action_id = cold_action_json->valuestring;
                } else if (strcmp(flag_value, "curious") == 0) {
                    const cJSON *curious_action_json = cJSON_GetObjectItemCaseSensitive(payload, "value_curious_action");
                    if (cJSON_IsString(curious_action_json)) next_action_id = curious_action_json->valuestring;
                }
            }

            if (next_action_id == NULL) { // Fallback to default if flag not found or no specific match
                const cJSON *default_action_json = cJSON_GetObjectItemCaseSensitive(payload, "default_action");
                if (cJSON_IsString(default_action_json)) next_action_id = default_action_json->valuestring;
            }

            if (next_action_id != NULL) {
                scene_changed = execute_action(next_action_id, game_state); // Recursively execute the chosen action
            }
        }
    } else {
        // Handle simple story_change, location_change, etc.
        const cJSON *story_file_json = cJSON_GetObjectItemCaseSensitive(payload, "story_file");
        if (cJSON_IsString(story_file_json) && story_file_json->valuestring != NULL) {
            strncpy(game_state->current_story_file, story_file_json->valuestring, MAX_PATH_LENGTH - 1);
            scene_changed = 1;
        }
    }

    const cJSON *new_location_json = cJSON_GetObjectItemCaseSensitive(payload, "new_location");
    if (cJSON_IsString(new_location_json) && new_location_json->valuestring != NULL) {
        strncpy(game_state->player_state.location, new_location_json->valuestring, MAX_NAME_LENGTH - 1);
    }


    // --- ACQUIRE ITEM ---
    if (strcmp(action->type_str, "acquire_item") == 0) {
        const cJSON *item_id_json = cJSON_GetObjectItemCaseSensitive(payload, "item_id");
        if (cJSON_IsString(item_id_json) && item_id_json->valuestring != NULL) {
            const Item* item_def = find_item_by_id(item_id_json->valuestring, game_state);
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
                }
            }
        }
    }
    
    // --- UNLOCK COMMANDS ---
    const cJSON *commands_json = cJSON_GetObjectItemCaseSensitive(payload, "commands");
    if (cJSON_IsArray(commands_json)) {
        cJSON *command_item;
        cJSON_ArrayForEach(command_item, commands_json) {
            if (cJSON_IsString(command_item) && command_item->valuestring != NULL) {
                int found = 0;
                for (int i = 0; i < game_state->player_state.unlocked_commands_count; i++) {
                    if (strcmp(game_state->player_state.unlocked_commands[i], command_item->valuestring) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found && game_state->player_state.unlocked_commands_count < MAX_COMMANDS) {
                    strncpy(game_state->player_state.unlocked_commands[game_state->player_state.unlocked_commands_count], command_item->valuestring, MAX_NAME_LENGTH - 1);
                    game_state->player_state.unlocked_commands_count++;
                }
            }
        }
    }
    
    // --- SET FLAGS ---
    const cJSON *flags_json = cJSON_GetObjectItemCaseSensitive(payload, "flags");
    if (cJSON_IsArray(flags_json)) {
        cJSON *flag_item;
        cJSON_ArrayForEach(flag_item, flags_json) {
            if (cJSON_IsObject(flag_item)) {
                const cJSON *name_json = cJSON_GetObjectItemCaseSensitive(flag_item, "name");
                const cJSON *value_json = cJSON_GetObjectItemCaseSensitive(flag_item, "value");

                if (cJSON_IsString(name_json) && name_json->valuestring != NULL && value_json != NULL) {
                    char value_str[256]; // Buffer to hold the string representation of the value

                    if (cJSON_IsString(value_json)) {
                        strncpy(value_str, value_json->valuestring, sizeof(value_str) - 1);
                    } else if (cJSON_IsNumber(value_json)) {
                        snprintf(value_str, sizeof(value_str), "%f", value_json->valuedouble);
                        // Trim trailing zeros and decimal point if it's an integer
                        char *p = strchr(value_str, '.');
                        if (p != NULL) {
                            char *end = p + strlen(p) - 1;
                            while (end > p && *end == '0') {
                                *end-- = '\0';
                            }
                            if (end == p) { // If only the decimal point is left
                                *end = '\0';
                            }
                        }
                    } else if (cJSON_IsTrue(value_json)) {
                        strncpy(value_str, "true", sizeof(value_str) - 1);
                    } else if (cJSON_IsFalse(value_json)) {
                        strncpy(value_str, "false", sizeof(value_str) - 1);
                    } else {
                        // Skip if value is not a supported type
                        continue;
                    }
                    
                    value_str[sizeof(value_str) - 1] = '\0'; // Ensure null-termination
                    hash_table_set(game_state->flags, name_json->valuestring, value_str);
                }
            }
        }
    }
    
    // --- TIME PROGRESSION ---
    const cJSON *time_cost_json = cJSON_GetObjectItemCaseSensitive(payload, "time_cost");
    if (cJSON_IsNumber(time_cost_json) && time_cost_json->valueint > 0) {
        game_state->time_of_day += time_cost_json->valueint;
        // Handle wrapping around midnight (1440 minutes in a day)
        if (game_state->time_of_day >= 1440) {
            game_state->time_of_day -= 1440; 
            // Here you could also increment a 'day_count' flag if needed
        }
    }

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
            printf("Location: %s\n", current_loc->name);
            printf("Description: %s\n", current_loc->description);
            printf("\nPoints of Interest:\n");
            if (current_loc->pois_count == 0) {
                printf("  (none)\n");
            }
            for (int i = 0; i < current_loc->pois_count; i++) {
#ifdef USE_DEBUG_LOGGING
                fprintf(stderr, "DEBUG: Arls: Printing POI '%s'\n", current_loc->pois[i].name);
#endif
                printf("  - %s\n", current_loc->pois[i].name);
            }
            printf("\nConnections:\n");
            if (current_loc->connection_count == 0) {
                printf("  (none)\n");
            }
            for (int i = 0; i < current_loc->connection_count; i++) {
                printf("  - %s\n", current_loc->connections[i]);
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