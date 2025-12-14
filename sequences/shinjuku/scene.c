#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "map_loader.h" // For helper functions (init_location, add_connection_to_location, add_poi_to_location)

// Define the number of locations in this layout
#define SHINJUKU_LAYOUT_ROOM_COUNT 2 // Shinjuku Station and Abandoned Site

int create_shinjuku_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* shinjuku_station = &all_locations[starting_index];
    Location* shinjuku_abandoned_site = &all_locations[starting_index + 1];

    // --- Shinjuku Station (新宿駅) ---
    init_location(shinjuku_station, "shinjuku_station", get_string_by_id(TEXT_SCENE_NAME_MIYANOSAKA_STATION), get_string_by_id(MAP_LOCATION_SHINJUKU_STATION_DESC)); // Reusing string ID for now
    add_connection_to_location(shinjuku_station, "explore_site", "shinjuku_abandoned_site", NULL, NULL, "SCENE_SHINJUKU_ABANDONED_SITE");
    add_connection_to_location(shinjuku_station, "home", "chisa_home", NULL, NULL, NULL); // Connection to Chisa's home

    // --- Shinjuku Abandoned Site (新宿的废弃工地) ---
    init_location(shinjuku_abandoned_site, "shinjuku_abandoned_site", get_string_by_id(TEXT_SCENE_NAME_SHINJUKU_ABANDONED_SITE), get_string_by_id(MAP_LOCATION_SHINJUKU_ABANDONED_SITE_DESC));
    add_connection_to_location(shinjuku_abandoned_site, "exit_site", "shinjuku_station", NULL, NULL, NULL); // Connect back to station
    
    // Add POIs if needed for the abandoned site
    add_poi_to_location(shinjuku_abandoned_site, "rusty_equipment", get_string_by_id(MAP_POI_SHINJUKU_ABANDONED_SITE_RUSTY_EQUIPMENT_NAME), get_string_by_id(MAP_POI_SHINJUKU_ABANDONED_SITE_RUSTY_EQUIPMENT_DESC), NULL);


    return SHINJUKU_LAYOUT_ROOM_COUNT;
}
