#include "data_loader.h"
#include "cJSON.h"
#include "flag_system.h"
#include "ecc_time.h"
#include "string_ids.h"
#include "string_id_names.h"
#include "string_table.h"
#include "characters/mika.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* g_embedded_strings[TEXT_COUNT];

DataLoaderStatus read_entire_file(const char* path, char** buffer_ptr, long* length_ptr) {
    if (!path || !buffer_ptr || !length_ptr) {
        return DATA_LOADER_ERROR_FILE_OPEN; // Or a new status for invalid args
    }

    *buffer_ptr = NULL;
    *length_ptr = 0;

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return DATA_LOADER_ERROR_FILE_OPEN;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length == -1) {
        fclose(file);
        return DATA_LOADER_ERROR_FILE_STAT;
    }

    char *buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fclose(file);
        return DATA_LOADER_ERROR_MEMORY_ALLOC;
    }

    if (fread(buffer, 1, length, file) != (size_t)length) {
        fclose(file);
        free(buffer);
        return DATA_LOADER_ERROR_FILE_READ;
    }

    buffer[length] = '\0';
    fclose(file);

    *buffer_ptr = buffer;
    *length_ptr = length;

    return DATA_LOADER_SUCCESS;
}

int load_string_table() {
    init_string_table(g_embedded_strings, TEXT_COUNT);
    return 1;
}

int load_player_state(const char* path, GameState* game_state) {
    if (game_state == NULL) return 0;
    
    game_state->flags = create_hash_table(128);
    if (game_state->flags == NULL) {
        return 0;
    }

    PlayerState* player_state = &game_state->player_state;
    char *json_string = NULL;
    long length = 0;

    DataLoaderStatus status = read_entire_file(path, &json_string, &length);
    if (status != DATA_LOADER_SUCCESS) {
        // Here, you could handle different errors differently.
        // For now, we just fail.
        return 0;
    }

    cJSON *root = cJSON_ParseWithLength(json_string, length);
    free(json_string); 

    if (root == NULL) {
        return 0;
    }

    cJSON* credit_level = cJSON_GetObjectItemCaseSensitive(root, "credit_level");
    if (cJSON_IsNumber(credit_level)) {
        player_state->credit_level = credit_level->valueint;
    }

    cJSON* persona_perm = cJSON_GetObjectItemCaseSensitive(root, "persona_permissions");
    if (cJSON_IsNumber(persona_perm)) {
        player_state->persona_permissions = (uint8_t)persona_perm->valueint;
    } else {
        // Default: Lain has full control (RWX), Shu is dormant (0)
        // Or should it be balanced? Let's start with Lain Normal.
        // Actually, based on prologue, maybe both exist? 
        // Let's safe default to Lain RWX (7).
        player_state->persona_permissions = 7; 
    }

    cJSON* location = cJSON_GetObjectItemCaseSensitive(root, "location");
    if (cJSON_IsString(location)) {
        strncpy(player_state->location, location->valuestring, MAX_NAME_LENGTH - 1);
    } else {
        strncpy(player_state->location, "iwakura_upper_hallway", MAX_NAME_LENGTH - 1);
    }

    const cJSON *typewriter_delay_json = cJSON_GetObjectItemCaseSensitive(root, "typewriter_delay");
    if (cJSON_IsNumber(typewriter_delay_json)) {
        game_state->typewriter_delay = (float)typewriter_delay_json->valuedouble;
    } else {
        game_state->typewriter_delay = 0.04f;
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
    else strncpy(game_state->current_story_file, "SCENE_00_ENTRY", MAX_PATH_LENGTH - 1);

    const cJSON *time_json = cJSON_GetObjectItemCaseSensitive(root, "time_of_day");
    if (cJSON_IsNumber(time_json)) {
        game_state->time_of_day = (uint32_t)time_json->valuedouble;
    } else {
        const uint32_t default_start_time_units = (2 * 24 * 60 * 60 * 16) + (20 * 60 * 60 * 16);
        game_state->time_of_day = encode_time_with_ecc(default_start_time_units);
    }

    const cJSON *doll_lain = cJSON_GetObjectItemCaseSensitive(root, "doll_state_lain_room");
    if (cJSON_IsNumber(doll_lain)) game_state->doll_state_lain_room = (int8_t)doll_lain->valueint;
    else game_state->doll_state_lain_room = DOLL_STATE_NORMAL;

    const cJSON *doll_mika = cJSON_GetObjectItemCaseSensitive(root, "doll_state_mika_room");
    if (cJSON_IsNumber(doll_mika)) game_state->doll_state_mika_room = (int8_t)doll_mika->valueint;
    else game_state->doll_state_mika_room = DOLL_STATE_NORMAL;

    // Load Mika State
    const cJSON *mika_state_json = cJSON_GetObjectItemCaseSensitive(root, "mika_state");
    if (cJSON_IsObject(mika_state_json)) {
        const cJSON *loc = cJSON_GetObjectItemCaseSensitive(mika_state_json, "current_location");
        const cJSON *manual = cJSON_GetObjectItemCaseSensitive(mika_state_json, "is_manually_positioned");
        const cJSON *sanity_json = cJSON_GetObjectItemCaseSensitive(mika_state_json, "sanity_level");
        
        int sanity_val = 0;
        if (cJSON_IsNumber(sanity_json)) {
            sanity_val = sanity_json->valueint;
        }

        if (cJSON_IsString(loc) && cJSON_IsBool(manual)) {
             strncpy(game_state->mika_location_storage, loc->valuestring, MAX_NAME_LENGTH - 1);
             restore_mika_state(game_state->mika_location_storage, cJSON_IsTrue(manual), sanity_val);
        } else {
             // Fallback if partially missing
             restore_mika_state("iwakura_mikas_room", false, sanity_val);
        }
    } else {
        // Default
        restore_mika_state("iwakura_mikas_room", false, 0);
    }

    DecodedTimeResult time_check = decode_time_with_ecc(game_state->time_of_day);
    if (time_check.status == DOUBLE_BIT_ERROR_DETECTED) {
        hash_table_set(game_state->flags, "TIME_GLITCH_ACTIVE", "1");
    } else {
        hash_table_set(game_state->flags, "TIME_GLITCH_ACTIVE", "0");
    }

    cJSON_Delete(root);
    return 1;
}

#include "items_data.h"

int load_items_data(GameState* game_state) {
    if (game_state == NULL) return 0;
    
    const char *json_string = ITEMS_JSON_DATA;
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        fprintf(stderr, "Error: Failed to parse embedded items.json data.\n");
        return 0;
    }

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

    if (game_state->flags != NULL) {
        free_hash_table(game_state->flags);
    }
    cleanup_string_table();
}

int save_game_state(const char* path, const GameState* game_state) {
    if (game_state == NULL) return 0;

    const PlayerState* p_state = &game_state->player_state;
    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;

    cJSON_AddStringToObject(root, "location", p_state->location);
    cJSON_AddNumberToObject(root, "credit_level", p_state->credit_level);
    cJSON_AddNumberToObject(root, "persona_permissions", p_state->persona_permissions);
    cJSON_AddStringToObject(root, "current_story_file", game_state->current_story_file);
    cJSON_AddNumberToObject(root, "time_of_day", game_state->time_of_day);
    cJSON_AddNumberToObject(root, "doll_state_lain_room", game_state->doll_state_lain_room);
    cJSON_AddNumberToObject(root, "doll_state_mika_room", game_state->doll_state_mika_room);

    // Save Mika State
    const CharacterMika* mika = get_mika_module();
    if (mika) {
        cJSON *mika_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(mika_obj, "current_location", mika->current_location_id ? mika->current_location_id : "unknown");
        cJSON_AddBoolToObject(mika_obj, "is_manually_positioned", mika->is_manually_positioned);
        cJSON_AddNumberToObject(mika_obj, "sanity_level", mika->sanity_level);
        cJSON_AddItemToObject(root, "mika_state", mika_obj);
    }

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