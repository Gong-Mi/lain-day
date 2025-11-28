#include "map_loader.h"
#include "cJSON.h"
#include "cmap.h" // Include our new CMap header
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
        fprintf(stderr, "DEBUG: Failed to open file for reading: %s\n", path); // Debug added
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


    fprintf(stderr, "DEBUG: Entering load_map_data for path: %s\n", map_dir_path);


    if (game_state == NULL) {


        fprintf(stderr, "ERROR: GameState is NULL in load_map_data.\n");


        return 0;


    }





    // --- CMap Integration: Create the hash map ---


    game_state->location_map = cmap_create(MAX_LOCATIONS);


    if (game_state->location_map == NULL) {


        fprintf(stderr, "ERROR: Failed to create location map hash table.\n");


        return 0;


    }


    fprintf(stderr, "DEBUG: CMap created with size %d.\n", MAX_LOCATIONS);





    DIR *d;


    struct dirent *dir;


    char full_path[MAX_PATH_LENGTH * 2];


    char file_path[MAX_PATH_LENGTH * 2];


    game_state->location_count = 0;





    d = opendir(map_dir_path);


    if (!d) {


        fprintf(stderr, "ERROR: Failed to open map directory: %s\n", map_dir_path);


        cmap_destroy(game_state->location_map); // Clean up on failure


        return 0;


    }


    fprintf(stderr, "DEBUG: Successfully opened map directory: %s\n", map_dir_path);





    while ((dir = readdir(d)) != NULL) {


        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {


            continue;


        }





        snprintf(full_path, sizeof(full_path), "%s/%s", map_dir_path, dir->d_name);





        struct stat st;


        if (stat(full_path, &st) == -1) {


            fprintf(stderr, "DEBUG: stat failed for %s. Skipping.\n", full_path);


            continue;


        }





        if (S_ISDIR(st.st_mode)) {


            fprintf(stderr, "DEBUG: Processing location directory: %s\n", dir->d_name);


            if (game_state->location_count >= MAX_LOCATIONS) {


                fprintf(stderr, "WARNING: Max locations reached. Breaking map loading loop.\n");


                break;


            }





            Location *current_location = &game_state->all_locations[game_state->location_count];


            memset(current_location, 0, sizeof(Location));





            strncpy(current_location->id, dir->d_name, MAX_NAME_LENGTH - 1);


            current_location->id[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination





            // --- Read name.txt ---


            snprintf(file_path, sizeof(file_path), "%s/name.txt", full_path);


            char *name_buffer = read_file_to_buffer(file_path);


            if (name_buffer) {


                size_t name_len = strlen(name_buffer);


                if (name_len > 0 && name_buffer[name_len - 1] == '\n') {


                    name_buffer[name_len - 1] = '\0';


                }


                strncpy(current_location->name, name_buffer, MAX_NAME_LENGTH - 1);


                current_location->name[MAX_NAME_LENGTH - 1] = '\0';


                free(name_buffer);


            } else {


                fprintf(stderr, "WARNING: Could not read name.txt for location %s. Path: %s\n", dir->d_name, file_path);


            }





            // --- Read description.txt ---


            snprintf(file_path, sizeof(file_path), "%s/description.txt", full_path);


            char *desc_buffer = read_file_to_buffer(file_path);


            if (desc_buffer) {


                size_t desc_len = strlen(desc_buffer);


                if (desc_len > 0 && desc_buffer[desc_len - 1] == '\n') {


                    desc_buffer[desc_len - 1] = '\0';


                }


                strncpy(current_location->description, desc_buffer, (MAX_DESC_LENGTH * 4) - 1);


                current_location->description[(MAX_DESC_LENGTH * 4) - 1] = '\0';


                free(desc_buffer);


            } else {


                fprintf(stderr, "WARNING: Could not read description.txt for location %s. Path: %s\n", dir->d_name, file_path);


            }





            // --- Read poi.json ---


            snprintf(file_path, sizeof(file_path), "%s/poi.json", full_path);


            char *poi_json_string = read_file_to_buffer(file_path);


            if (poi_json_string) {


                cJSON *poi_root = cJSON_Parse(poi_json_string);


                if (!poi_root) { // Debug added for cJSON_Parse failure


                    fprintf(stderr, "ERROR: cJSON_Parse failed for poi.json in %s. Path: %s\n", dir->d_name, file_path);


                } else if (cJSON_IsArray(poi_root)) {


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


                                    current_poi->id[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination


                                }


                                if (cJSON_IsString(name_json) && name_json->valuestring != NULL) {


                                    strncpy(current_poi->name, name_json->valuestring, MAX_NAME_LENGTH - 1);


                                    current_poi->name[MAX_NAME_LENGTH - 1] = '\0';


                                } else if (cJSON_IsString(id_json) && id_json->valuestring != NULL) { // Fallback name to id if name is missing


                                    strncpy(current_poi->name, id_json->valuestring, MAX_NAME_LENGTH - 1);


                                    current_poi->name[MAX_NAME_LENGTH - 1] = '\0';


                                }


                                if (cJSON_IsString(desc_json) && desc_json->valuestring != NULL) {


                                    strncpy(current_poi->description, desc_json->valuestring, (MAX_DESC_LENGTH * 2) - 1);


                                    current_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';


                                }


                            } else if (cJSON_IsString(poi_item)) { // Handle old simple string POI format


                                strncpy(current_poi->id, poi_item->valuestring, MAX_NAME_LENGTH - 1);


                                current_poi->id[MAX_NAME_LENGTH - 1] = '\0';


                                strncpy(current_poi->name, poi_item->valuestring, MAX_NAME_LENGTH - 1);


                                current_poi->name[MAX_NAME_LENGTH - 1] = '\0';


                                strncpy(current_poi->description, "No detailed description.", (MAX_DESC_LENGTH * 2) - 1);


                                current_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';


                            }


                            current_location->pois_count++;


                        }


                    }


                } else {


                    fprintf(stderr, "WARNING: poi.json for location %s is not a JSON array or invalid. Path: %s\n", dir->d_name, file_path);


                }


                cJSON_Delete(poi_root);


                free(poi_json_string);


            } else {


                fprintf(stderr, "WARNING: Could not read or parse poi.json for location %s. Path: %s\n", dir->d_name, file_path);


            }





            // --- Read connections.json ---


            snprintf(file_path, sizeof(file_path), "%s/connections.json", full_path);


            char *connections_json_string = read_file_to_buffer(file_path);


            if (connections_json_string) {


                cJSON *connections_root = cJSON_Parse(connections_json_string);


                if (!connections_root) { // Debug added for cJSON_Parse failure


                    fprintf(stderr, "ERROR: cJSON_Parse failed for connections.json in %s. Path: %s\n", dir->d_name, file_path);


                } else if (cJSON_IsObject(connections_root)) {


                    cJSON *conn_item;


                    cJSON_ArrayForEach(conn_item, connections_root) {


                        if (cJSON_IsString(conn_item) && current_location->connection_count < MAX_CONNECTIONS) {


                            strncpy(current_location->connections[current_location->connection_count], conn_item->valuestring, MAX_NAME_LENGTH - 1);


                            current_location->connections[current_location->connection_count][MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination


                            current_location->connection_count++;


                        }


                    }


                } else {


                    fprintf(stderr, "WARNING: connections.json for location %s is not a JSON object or invalid. Path: %s\n", dir->d_name, file_path);


                }


                cJSON_Delete(connections_root);


                free(connections_json_string);


            } else {


                fprintf(stderr, "WARNING: Could not read or parse connections.json for location %s. Path: %s\n", dir->d_name, file_path);


            }





            cmap_insert(game_state->location_map, (struct Location*)current_location);


            fprintf(stderr, "DEBUG: Inserted location '%s' into CMap.\n", current_location->id);





            game_state->location_count++;


        }


    }


    closedir(d);


    fprintf(stderr, "DEBUG: Successfully loaded %d locations from map data.\n", game_state->location_count);


    return 1;


}