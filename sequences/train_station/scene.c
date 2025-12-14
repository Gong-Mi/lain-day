#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "map_loader.h" // for init_location and add_connection_to_location
#include "string_table.h" // for get_string_by_id
#include "station_coordinates_data.h" // For embedded station_coordinates.json data

// No need for game_paths.h or data_loader.h includes anymore here

int create_train_station_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    const char *json_string = STATION_COORDINATES_JSON_DATA;

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
             fprintf(stderr, "Error: Failed to parse embedded station_coordinates.json before: %s\n", error_ptr);
        } else {
             fprintf(stderr, "Error: Failed to parse embedded station_coordinates.json (unknown error).\n");
        }
        return 0;
    }

    if (!cJSON_IsArray(root)) {
        fprintf(stderr, "Error: station_coordinates.json is not a JSON array.\n");
        cJSON_Delete(root);
        return 0;
    }

    int num_stations = cJSON_GetArraySize(root);
    if (num_stations == 0) {
        fprintf(stderr, "Warning: No stations found in station_coordinates.json.\n");
        cJSON_Delete(root);
        return 0;
    }

    // Allocate memory for station IDs if needed, or directly use all_locations
    // For this implementation, we will directly populate all_locations
    if (starting_index + num_stations >= MAX_LOCATIONS) {
        fprintf(stderr, "Error: Not enough space in all_locations for all train stations.\n");
        cJSON_Delete(root);
        return 0;
    }

    // Create a temporary array to store station IDs for easy lookup when creating connections
    const char** station_ids = (const char**)malloc(sizeof(const char*) * num_stations);
    if (station_ids == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for station_ids.\n");
        cJSON_Delete(root);
        return 0;
    }


    for (int i = 0; i < num_stations; i++) {
        cJSON *station_json = cJSON_GetArrayItem(root, i);
        cJSON *id_json = cJSON_GetObjectItemCaseSensitive(station_json, "id");
        cJSON *name_json = cJSON_GetObjectItemCaseSensitive(station_json, "name");

        if (cJSON_IsString(id_json) && cJSON_IsString(name_json)) {
            Location* current_station = &all_locations[starting_index + i];
            init_location(current_station, id_json->valuestring, name_json->valuestring, get_string_by_id(MAP_LOCATION_TRAIN_STATION_GENERIC_DESC)); // Generic description
            
            // Add a POI for the ticket machine
            add_poi_to_location(current_station, "ticket_machine", get_string_by_id(MAP_POI_TRAIN_STATION_TICKET_MACHINE_NAME), get_string_by_id(MAP_POI_TRAIN_STATION_TICKET_MACHINE_DESC), "use_ticket_machine");

            station_ids[i] = current_station->id; // Store ID for connections
        }
    }

    // Create circular connections (Yamanote line style)
    for (int i = 0; i < num_stations; i++) {
        Location* current_station = &all_locations[starting_index + i];
        
        // Connect to next station
        int next_idx = (i + 1) % num_stations;
        add_connection_to_location(current_station, "go_next_station", station_ids[next_idx], NULL, NULL, NULL);

        // Connect to previous station
        int prev_idx = (i - 1 + num_stations) % num_stations;
        add_connection_to_location(current_station, "go_prev_station", station_ids[prev_idx], NULL, NULL, NULL);
    }
    
    // Add a connection for miyanosaka_station to its adjacent street
    // This part is a bit hardcoded and assumes specific connections.
    // In a more robust system, this might be defined in the JSON or another map config.
    // For now, let's assume "miyanosaka_station" connects to "miyanosaka_street"
    // We need to find "miyanosaka_station" in our newly created locations and add the connection.
    for (int i = 0; i < num_stations; i++) {
        Location* station = &all_locations[starting_index + i];
        if (strcmp(station->id, "miyanosaka_station") == 0) {
            add_connection_to_location(station, "exit_station", "miyanosaka_street", NULL, NULL, NULL);
            break;
        }
    }


    free(station_ids);
    cJSON_Delete(root);
    return num_stations;
}