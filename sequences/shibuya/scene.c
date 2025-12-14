#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "map_loader.h"

#define SHIBUYA_LAYOUT_ROOM_COUNT 1

int create_shibuya_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* shibuya_street = &all_locations[starting_index];

    // --- Shibuya Street ---
    init_location(shibuya_street, "shibuya_street", get_string_by_id(MAP_LOCATION_SHIBUYA_STREET_NAME), get_string_by_id(MAP_LOCATION_SHIBUYA_STREET_DESC));
    
    // Connections
    add_connection_to_location(shibuya_street, "enter_cyberia", "cyberia_club", NULL, NULL, "SCENE_09_CYBERIA");
    add_connection_to_location(shibuya_street, "go_to_station", "shibuya", NULL, NULL, NULL); // Train station "shibuya" created by train system
    add_connection_to_location(shibuya_street, "take_subway_to_roppongi", "roppongi_street", NULL, NULL, NULL);

    // POIs
    add_poi_to_location(shibuya_street, "crossing", get_string_by_id(MAP_POI_SHIBUYA_STREET_CROSSING_NAME), get_string_by_id(MAP_POI_SHIBUYA_STREET_CROSSING_DESC), NULL);

    return SHIBUYA_LAYOUT_ROOM_COUNT;
}
