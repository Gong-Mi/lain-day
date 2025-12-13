#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "map_loader.h" // For helper functions

int create_miyanosaka_station_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* miyanosaka_station = &all_locations[starting_index];
    init_location(miyanosaka_station, "miyanosaka_station", get_string_by_id(TEXT_SCENE_NAME_MIYANOSAKA_STATION), get_string_by_id(MAP_LOCATION_MIYANOSAKA_STATION_DESC));
    
    add_connection_to_location(miyanosaka_station, "miyanosaka_street", "miyanosaka_street", NULL, NULL, NULL);
    add_connection_to_location(miyanosaka_station, "shibuya", "shibuya_street", NULL, NULL, "SCENE_09_CYBERIA");

    return 1; // 1 room added
}
