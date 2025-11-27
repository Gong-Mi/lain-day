#include "map_loader.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h> 
#include <ctype.h>

// Helper function to read a whole file into a string buffer.
// The caller is responsible for freeing the returned buffer.
static char* read_file_to_buffer(const char* path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length == -1) {
        fclose(file);
        return NULL;
    }

    char *buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, length, file) != (size_t)length) {
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(file);
    return buffer;
}


int load_map_data(const char* map_dir_path, GameState* game_state) {
    if (game_state == NULL) {
        return 0;
    }

    DIR *d;
    struct dirent *dir;
    char full_path[MAX_PATH_LENGTH * 2];
    char file_path[MAX_PATH_LENGTH * 2];
    game_state->location_count = 0;

    d = opendir(map_dir_path);
    if (!d) {
        return 0;
    }

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", map_dir_path, dir->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (game_state->location_count >= MAX_LOCATIONS) {
                break;
            }

            Location *current_location = &game_state->all_locations[game_state->location_count];
            memset(current_location, 0, sizeof(Location));

            strncpy(current_location->id, dir->d_name, MAX_NAME_LENGTH - 1);

            snprintf(file_path, sizeof(file_path), "%s/name.txt", full_path);
            char *name_buffer = read_file_to_buffer(file_path);
            if (name_buffer) {
                size_t name_len = strlen(name_buffer);
                if (name_len > 0 && name_buffer[name_len - 1] == '\n') {
                    name_buffer[name_len - 1] = '\0';
                }
                strncpy(current_location->name, name_buffer, MAX_NAME_LENGTH - 1);
                free(name_buffer);
            }

            snprintf(file_path, sizeof(file_path), "%s/description.txt", full_path);
            char *desc_buffer = read_file_to_buffer(file_path);
            if (desc_buffer) {
                size_t desc_len = strlen(desc_buffer);
                if (desc_len > 0 && desc_buffer[desc_len - 1] == '\n') {
                    desc_buffer[desc_len - 1] = '\0';
                }
                strncpy(current_location->description, desc_buffer, (MAX_DESC_LENGTH * 4) - 1);
                free(desc_buffer);
            }

            snprintf(file_path, sizeof(file_path), "%s/poi.json", full_path);
            char *poi_json_string = read_file_to_buffer(file_path);
            if (poi_json_string) {
                cJSON *poi_root = cJSON_Parse(poi_json_string);
                if (cJSON_IsArray(poi_root)) {
                    cJSON *poi_item;
                    cJSON_ArrayForEach(poi_item, poi_root) {
                        if (current_location->pois_count < MAX_POIS) {
                            POI *current_poi = &current_location->pois[current_location->pois_count];
                            memset(current_poi, 0, sizeof(POI)); // Initialize POI struct

                            if (cJSON_IsObject(poi_item)) {
                                const cJSON *id_json = cJSON_GetObjectItemCaseSensitive(poi_item, "id");
                                const cJSON *name_json = cJSON_GetObjectItemCaseSensitive(poi_item, "name");
                                const cJSON *desc_json = cJSON_GetObjectItemCaseSensitive(poi_item, "description");

                                if (cJSON_IsString(id_json) && id_json->valuestring != NULL) {
                                    strncpy(current_poi->id, id_json->valuestring, MAX_NAME_LENGTH - 1);
                                }
                                if (cJSON_IsString(name_json) && name_json->valuestring != NULL) {
                                    strncpy(current_poi->name, name_json->valuestring, MAX_NAME_LENGTH - 1);
                                } else if (cJSON_IsString(id_json) && id_json->valuestring != NULL) { // Fallback name to id if name is missing
                                    strncpy(current_poi->name, id_json->valuestring, MAX_NAME_LENGTH - 1);
                                }
                                if (cJSON_IsString(desc_json) && desc_json->valuestring != NULL) {
                                    strncpy(current_poi->description, desc_json->valuestring, (MAX_DESC_LENGTH * 2) - 1);
                                }
                            } else if (cJSON_IsString(poi_item)) { // Handle old simple string POI format
                                strncpy(current_poi->id, poi_item->valuestring, MAX_NAME_LENGTH - 1);
                                strncpy(current_poi->name, poi_item->valuestring, MAX_NAME_LENGTH - 1);
                                strncpy(current_poi->description, "No detailed description.", (MAX_DESC_LENGTH * 2) - 1);
                            }
                            current_location->pois_count++;
                        }
                    }
                }
                cJSON_Delete(poi_root);
                free(poi_json_string);
            }

            snprintf(file_path, sizeof(file_path), "%s/connections.json", full_path);
            char *connections_json_string = read_file_to_buffer(file_path);
            if (connections_json_string) {
                cJSON *connections_root = cJSON_Parse(connections_json_string);
                if (cJSON_IsObject(connections_root)) {
                    cJSON *conn_item;
                    cJSON_ArrayForEach(conn_item, connections_root) {
                        if (cJSON_IsString(conn_item) && current_location->connection_count < MAX_CONNECTIONS) {
                            strncpy(current_location->connections[current_location->connection_count], conn_item->valuestring, MAX_NAME_LENGTH - 1);
                            current_location->connection_count++;
                        }
                    }
                }
                cJSON_Delete(connections_root);
                free(connections_json_string);
            }

            game_state->location_count++;
        }
    }
    closedir(d);
    return 1;
}