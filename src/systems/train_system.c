#include "train_system.h"
#include "game_types.h"
#include "data_loader.h"
#include "cJSON.h"
#include "linenoise.h" // For input
#include "string_table.h" // For get_string_by_id
#include "ansi_colors.h" // For rendering
#include "render_utils.h" // For render_clear_screen
#include "game_paths.h" // For GamePaths struct and global game_state
#include "station_coordinates_data.h" // For embedded station_coordinates.json data
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper struct for train station data, mirroring station_coordinates.json
typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    // We don't need x, y for this initial implementation of the interface
} TrainStationData;

// Global array to store parsed station data
static TrainStationData* g_stations = NULL;
static int g_station_count = 0;

/**
 * @brief Loads train station data from station_coordinates.json.
 * @return 1 on success, 0 on failure.
 */
static int load_station_data() {
    const char *json_string = STATION_COORDINATES_JSON_DATA;

    // No need for file path or read_entire_file anymore
    // No need for length since it's a static string
    // DataLoaderStatus status; // No longer needed
    // length = strlen(json_string); // No longer needed
    // json_string will be valid as it's embedded

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error: Failed to parse embedded station_coordinates.json before: %s\n", error_ptr);
        } else {
            fprintf(stderr, "Error: Failed to parse embedded station_coordinates.json (unknown cJSON error).\n");
        }
        return 0;
    }

    if (!cJSON_IsArray(root)) {
        fprintf(stderr, "Error: station_coordinates.json is not a JSON array.\n");
        cJSON_Delete(root);
        return 0;
    }

    g_station_count = cJSON_GetArraySize(root);
    if (g_station_count == 0) {
        fprintf(stderr, "Warning: No stations found in station_coordinates.json.\n");
        cJSON_Delete(root);
        return 0;
    }

    g_stations = (TrainStationData*)malloc(sizeof(TrainStationData) * g_station_count);
    if (g_stations == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for station data.\n");
        cJSON_Delete(root);
        return 0;
    }

    for (int i = 0; i < g_station_count; i++) {
        cJSON *station_json = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(station_json)) {
            fprintf(stderr, "Error: Invalid station entry in station_coordinates.json at index %d.\n", i);
            cJSON_Delete(root);
            free(g_stations);
            g_stations = NULL;
            g_station_count = 0;
            return 0;
        }

        cJSON *id_json = cJSON_GetObjectItemCaseSensitive(station_json, "id");
        cJSON *name_json = cJSON_GetObjectItemCaseSensitive(station_json, "name");

        if (cJSON_IsString(id_json) && cJSON_IsString(name_json)) {
            strncpy(g_stations[i].id, id_json->valuestring, MAX_NAME_LENGTH - 1);
            strncpy(g_stations[i].name, name_json->valuestring, MAX_NAME_LENGTH - 1);
        } else {
            fprintf(stderr, "Error: Missing 'id' or 'name' in station entry at index %d.\n", i);
            cJSON_Delete(root);
            free(g_stations);
            g_stations = NULL;
            g_station_count = 0;
            return 0;
        }
    }

    cJSON_Delete(root);
    return 1;
}

void enter_ticket_machine_interface(GameState* passed_game_state) {

    if (passed_game_state == NULL) return;



    if (g_stations == NULL && !load_station_data()) {

        fprintf(stderr, "ERROR: Could not load train station data for ticket machine.\n");

        return;

    }



    int running = 1;

    char* input = NULL;



        clear_screen();

    printf("         东京电车购票机\n");

    printf("========================================\n");

    printf("请选择您的目的地：\n");

    for (int i = 0; i < g_station_count; i++) {

        printf("  %d. %s (%s)\n", i + 1, g_stations[i].name, g_stations[i].id);

    }

    printf("----------------------------------------\n");

    printf("  e. 退出\n");

    printf("========================================\n");



    while (running) {

        input = linenoise(get_string_by_id(TEXT_PROMPT_INPUT_ARROW));

        if (input == NULL) { // Ctrl+D

            running = 0;

            continue;

        }



        if (strlen(input) == 0) {

            free(input);

            continue;

        }



        if (strcmp(input, "e") == 0) {

            running = 0;

        } else {

            // Placeholder: In a real implementation, we'd validate input,

            // process ticket purchase/travel, and update game state.

            printf("您选择了: %s。当前功能开发中，请选择 'e' 退出。\n", input);

        }

        free(input);

        input = NULL; // Reset input pointer

    }

    if (input) free(input); // Free if loop exited without input being freed

    

    // Cleanup station data when exiting the interface, or manage it globally

    // For now, keep it loaded as it might be used by train_station/scene.c too.

    // if (g_stations) {

    //     free(g_stations);

    //     g_stations = NULL;

    //     g_station_count = 0;

    // }

    clear_screen();

}




