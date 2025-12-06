#include "data_loader.h"
#include "cJSON.h"
#include "flag_system.h"
#include "ecc_time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file_to_buffer(const char* path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Failed to open file: %s\n", path);
#endif
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length == -1) { fclose(file); return NULL; }
    char *buffer = (char*)malloc(length + 1);
    if (buffer == NULL) { fclose(file); return NULL; }
    if (fread(buffer, 1, length, file) != (size_t)length) { fclose(file); free(buffer); return NULL; }
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

int load_player_state(const char* path, GameState* game_state) {
    if (game_state == NULL) return 0;
    
    // Initialize the flag system
    game_state->flags = create_hash_table(128);
    if (game_state->flags == NULL) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Failed to create hash table for flags.\n");
#endif
        return 0;
    }

    PlayerState* player_state = &game_state->player_state;
    char *json_string = read_file_to_buffer(path);
    if (json_string == NULL) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Failed to read file to buffer: %s\n", path);
#endif
        return 0;
    }
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Read JSON string:\n%s\n", json_string);
#endif
    cJSON *root = cJSON_Parse(json_string);
    free(json_string); 
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
#ifdef USE_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: cJSON_Parse error before: %s\n", error_ptr);
#endif
        }
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Failed to parse JSON from %s\n", path);
#endif
        return 0;
    }
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Successfully parsed JSON from %s\n", path);
#endif

    const cJSON *location = cJSON_GetObjectItemCaseSensitive(root, "location");
    if (cJSON_IsString(location)) strncpy(player_state->location, location->valuestring, MAX_NAME_LENGTH - 1);

    const cJSON *credit_level = cJSON_GetObjectItemCaseSensitive(root, "credit_level");
    if (cJSON_IsNumber(credit_level)) player_state->credit_level = credit_level->valueint;

    // Load typewriter delay, with a default value
    const cJSON *typewriter_delay_json = cJSON_GetObjectItemCaseSensitive(root, "typewriter_delay");
    if (cJSON_IsNumber(typewriter_delay_json)) {
        game_state->typewriter_delay = (float)typewriter_delay_json->valuedouble;
    } else {
        game_state->typewriter_delay = 0.04f; // Default value
    }

    const cJSON *inventory = cJSON_GetObjectItemCaseSensitive(root, "inventory");
    player_state->inventory_count = 0;
    if (cJSON_IsObject(inventory)) {
        cJSON *item_json;
        cJSON_ArrayForEach(item_json, inventory) {
            if (player_state->inventory_count < MAX_INVENTORY_ITEMS) {
                InventoryItem *item = &player_state->inventory[player_state->inventory_count];
                strncpy(item->name, item_json->string, MAX_NAME_LENGTH - 1);
                item->quantity = cJSON_IsNumber(item_json) ? item_json->valueint : 0;
                player_state->inventory_count++;
            }
        }
    }

    const cJSON *commands = cJSON_GetObjectItemCaseSensitive(root, "unlocked_commands");
    player_state->unlocked_commands_count = 0;
    if (cJSON_IsArray(commands)) {
        cJSON *command_json;
        cJSON_ArrayForEach(command_json, commands) {
            if (cJSON_IsString(command_json) && player_state->unlocked_commands_count < MAX_COMMANDS) {
                strncpy(player_state->unlocked_commands[player_state->unlocked_commands_count], command_json->valuestring, MAX_NAME_LENGTH - 1);
                player_state->unlocked_commands_count++;
            }
        }
    }

    const cJSON *story_file = cJSON_GetObjectItemCaseSensitive(root, "current_story_file");
    if (cJSON_IsString(story_file)) strncpy(game_state->current_story_file, story_file->valuestring, MAX_PATH_LENGTH - 1);
    else strncpy(game_state->current_story_file, "story/00_entry.md", MAX_PATH_LENGTH - 1);

    const cJSON *time_json = cJSON_GetObjectItemCaseSensitive(root, "time_of_day");
    if (cJSON_IsNumber(time_json)) {
        game_state->time_of_day = (uint32_t)time_json->valuedouble;
    } else {
        // Initialize with a default start time (Day 3, 20:00) if not found
        const uint32_t default_start_time_units = (2 * 24 * 60 * 60 * 16) + (20 * 60 * 60 * 16); // Day 3, 8 PM
        game_state->time_of_day = encode_time_with_ecc(default_start_time_units);
    }

    // Immediately check the loaded time for corruption
    DecodedTimeResult time_check = decode_time_with_ecc(game_state->time_of_day);
    if (time_check.status == DOUBLE_BIT_ERROR_DETECTED) {
        hash_table_set(game_state->flags, "TIME_GLITCH_ACTIVE", "1");
        #ifdef USE_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Corrupted time detected! TIME_GLITCH_ACTIVE flag set.\n");
        #endif
    } else {
        // Ensure the flag is not set if time is okay
        hash_table_set(game_state->flags, "TIME_GLITCH_ACTIVE", "0");
    }

    cJSON_Delete(root);
    return 1;
}

int load_items_data(const char* path, GameState* game_state) {
    if (game_state == NULL) return 0;
    char *json_string = read_file_to_buffer(path);
    if (json_string == NULL) return 0;

    cJSON *root = cJSON_Parse(json_string);
    free(json_string);
    if (root == NULL) return 0;

    game_state->item_count = 0;
    cJSON *item_json;
    cJSON_ArrayForEach(item_json, root) {
        if (game_state->item_count >= MAX_ITEMS) break;

        Item *item = &game_state->all_items[game_state->item_count];
        strncpy(item->id, item_json->string, MAX_NAME_LENGTH - 1);

        const cJSON *name = cJSON_GetObjectItemCaseSensitive(item_json, "name");
        if (cJSON_IsString(name)) strncpy(item->name, name->valuestring, MAX_NAME_LENGTH - 1);

        const cJSON *desc = cJSON_GetObjectItemCaseSensitive(item_json, "description");
        if (cJSON_IsString(desc)) strncpy(item->description, desc->valuestring, MAX_DESC_LENGTH - 1);

        const cJSON *credit = cJSON_GetObjectItemCaseSensitive(item_json, "required_credit");
        if (cJSON_IsNumber(credit)) item->required_credit = credit->valueint;

        game_state->item_count++;
    }

    cJSON_Delete(root);
    return 1;
}

void cleanup_game_state(GameState* game_state) {
    if (game_state == NULL) return;

    // Free the flag system hash table
    if (game_state->flags != NULL) {
        free_hash_table(game_state->flags);
    }
}

int save_game_state(const char* path, const GameState* game_state) {
    if (game_state == NULL) return 0;

    const PlayerState* p_state = &game_state->player_state;
    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;

    cJSON_AddStringToObject(root, "location", p_state->location);
    cJSON_AddNumberToObject(root, "credit_level", p_state->credit_level);
    cJSON_AddStringToObject(root, "current_story_file", game_state->current_story_file);
    cJSON_AddNumberToObject(root, "time_of_day", game_state->time_of_day);

    cJSON *inv = cJSON_CreateObject();
    if (inv) {
        cJSON_AddItemToObject(root, "inventory", inv);
        for (int i = 0; i < p_state->inventory_count; i++) {
            cJSON_AddNumberToObject(inv, p_state->inventory[i].name, p_state->inventory[i].quantity);
        }
    }

    cJSON *cmds = cJSON_CreateArray();
    if (cmds) {
        cJSON_AddItemToObject(root, "unlocked_commands", cmds);
        for (int i = 0; i < p_state->unlocked_commands_count; i++) {
            cJSON *cmd_str = cJSON_CreateString(p_state->unlocked_commands[i]);
            if (cmd_str) cJSON_AddItemToArray(cmds, cmd_str);
        }
    }
    
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_string) return 0;

    FILE *file = fopen(path, "w");
    if (!file) {
        free(json_string);
        return 0;
    }

    fprintf(file, "%s", json_string);
    fclose(file);
    free(json_string);

    return 1;
}