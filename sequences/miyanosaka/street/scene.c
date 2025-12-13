#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "map_loader.h" // For helper functions

int create_miyanosaka_street_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* miyanosaka_street = &all_locations[starting_index];
    init_location(miyanosaka_street, "miyanosaka_street", get_string_by_id(MAP_LOCATION_MIYANOSAKA_STREET_NAME), get_string_by_id(MAP_LOCATION_MIYANOSAKA_STREET_DESC));
    
    add_poi_to_location(miyanosaka_street, "vending_machine", get_string_by_id(MAP_POI_MIYANOSAKA_STREET_VENDING_MACHINE_NAME), get_string_by_id(MAP_POI_MIYANOSAKA_STREET_VENDING_MACHINE_DESC), NULL);
    
    add_connection_to_location(miyanosaka_street, "iwakura_residence", "iwakura_front_yard", NULL, NULL, "SCENE_00_ENTRY");
    add_connection_to_location(miyanosaka_street, "train_station", "miyanosaka_station", NULL, NULL, NULL);
    add_connection_to_location(miyanosaka_street, "go_to_park", "miyanosaka_park", NULL, NULL, NULL);
    add_connection_to_location(miyanosaka_street, "go_to_center_park", "miyasaka_center_park", NULL, NULL, NULL);

    Location* miyanosaka_park = &all_locations[starting_index + 1];
    init_location(miyanosaka_park, "miyanosaka_park", get_string_by_id(MAP_LOCATION_WAKABAYASHI_PARK_NAME), get_string_by_id(MAP_LOCATION_WAKABAYASHI_PARK_DESC));
    add_connection_to_location(miyanosaka_park, "return_to_street", "miyanosaka_street", NULL, NULL, NULL);

    Location* miyasaka_center_park = &all_locations[starting_index + 2];
    init_location(miyasaka_center_park, "miyasaka_center_park", get_string_by_id(MAP_LOCATION_MIYASAKA_CENTER_PARK_NAME), get_string_by_id(MAP_LOCATION_MIYASAKA_CENTER_PARK_DESC));
    add_connection_to_location(miyasaka_center_park, "return_to_street", "miyanosaka_street", NULL, NULL, NULL);

    return 3; // 3 rooms added
}
