#include "data_loader.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file_to_buffer(const char* path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "DEBUG: Failed to open file: %s\n", path);
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
    PlayerState* player_state = &game_state->player_state;
    char *json_string = read_file_to_buffer(path);
    if (json_string == NULL) {
        fprintf(stderr, "DEBUG: Failed to read file to buffer: %s\n", path);
        return 0;
    }
    fprintf(stderr, "DEBUG: Read JSON string:\n%s\n", json_string);
    cJSON *root = cJSON_Parse(json_string);
    free(json_string); 
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "DEBUG: cJSON_Parse error before: %s\n", error_ptr);
        }
        fprintf(stderr, "DEBUG: Failed to parse JSON from %s\n", path);
        return 0;
    }
    fprintf(stderr, "DEBUG: Successfully parsed JSON from %s\n", path);

    const cJSON *location = cJSON_GetObjectItemCaseSensitive(root, "location");
    if (cJSON_IsString(location)) strncpy(player_state->location, location->valuestring, MAX_NAME_LENGTH - 1);

    const cJSON *credit_level = cJSON_GetObjectItemCaseSensitive(root, "credit_level");
    if (cJSON_IsNumber(credit_level)) player_state->credit_level = credit_level->valueint;

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

int load_actions_data(const char* path, GameState* game_state) {
    if (game_state == NULL) return 0;
    char *json_string = read_file_to_buffer(path);
    if (json_string == NULL) return 0;

    cJSON *root = cJSON_Parse(json_string);
    free(json_string);
    if (root == NULL) return 0;

    game_state->action_count = 0;
    cJSON *action_json;
    cJSON_ArrayForEach(action_json, root) {
        if (game_state->action_count >= MAX_ACTIONS) break;

        Action *action = &game_state->all_actions[game_state->action_count];
        strncpy(action->id, action_json->string, MAX_NAME_LENGTH - 1);

        const cJSON *type = cJSON_GetObjectItemCaseSensitive(action_json, "type");
        if (cJSON_IsString(type)) strncpy(action->type_str, type->valuestring, MAX_NAME_LENGTH - 1);

        cJSON *payload = cJSON_GetObjectItemCaseSensitive(action_json, "payload");
        if (payload) action->payload_json = cJSON_DetachItemViaPointer(action_json, payload);

        game_state->action_count++;
    }

    cJSON_Delete(root);
    return 1;
}

void cleanup_game_state(GameState* game_state) {
    if (game_state == NULL) return;
    for (int i = 0; i < game_state->action_count; i++) {
        if (game_state->all_actions[i].payload_json != NULL) {
            cJSON_Delete(game_state->all_actions[i].payload_json);
        }
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