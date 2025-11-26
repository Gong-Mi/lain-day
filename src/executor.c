#include "executor.h"
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

// Helper to find a location by its ID
static const Location* find_location_by_id(const char* location_id, const GameState* game_state) {
    for (int i = 0; i < game_state->location_count; i++) {
        if (strcmp(game_state->all_locations[i].id, location_id) == 0) {
            return &game_state->all_locations[i];
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
    const cJSON *story_file_json = cJSON_GetObjectItemCaseSensitive(payload, "story_file");
    if (cJSON_IsString(story_file_json) && story_file_json->valuestring != NULL) {
        strncpy(game_state->current_story_file, story_file_json->valuestring, MAX_PATH_LENGTH - 1);
        scene_changed = 1;
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
                const cJSON *name = cJSON_GetObjectItemCaseSensitive(flag_item, "name");
                const cJSON *value = cJSON_GetObjectItemCaseSensitive(flag_item, "value");
                if (cJSON_IsString(name) && name->valuestring != NULL) {
                    if (strcmp(name->valuestring, "credit_level") == 0 && cJSON_IsNumber(value)) {
                        game_state->player_state.credit_level = value->valueint;
                    }
                }
            }
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
        const Location* current_loc = find_location_by_id(game_state->player_state.location, game_state);
        if (current_loc) {
            printf("Location: %s\n", current_loc->name);
            printf("Description: %s\n", current_loc->description);
            printf("\nPoints of Interest:\n");
            if (current_loc->poi_count == 0) {
                printf("  (none)\n");
            }
            for (int i = 0; i < current_loc->poi_count; i++) {
                printf("  - %s\n", current_loc->points_of_interest[i]);
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