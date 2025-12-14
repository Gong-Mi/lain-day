#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "conditions.h" // For potential future accessibility functions
#include "flag_system.h" // For potential future flag checks
#include "game_types.h" // For MAX_NAME_LENGTH, MAX_DESC_LENGTH etc.
#include "map_loader.h" // For global helper functions
#include "string_table.h" // For get_string_by_id

int create_chisa_home_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* chisa_home = &all_locations[starting_index];
    *chisa_home = (Location){0}; // Zero out the struct

    strcpy(chisa_home->id, "chisa_home");
    strcpy(chisa_home->name, get_string_by_id(MAP_LOCATION_CHISA_HOME_NAME));
    strcpy(chisa_home->description, get_string_by_id(MAP_LOCATION_CHISA_HOME_DESC));

    // Connections (from old_map/chisa_home/connections.json - which was empty)
    // Add a default connection to Shinjuku Station for now
    add_connection_to_location(chisa_home, "exit_home", "shinjuku_station", NULL, NULL, NULL);

    // POIs from old_map/chisa_home/poi.json
    add_poi_to_location(chisa_home, "photo_on_door", get_string_by_id(MAP_POI_CHISA_HOME_PHOTO_NAME), get_string_by_id(MAP_POI_CHISA_HOME_PHOTO_DESC), NULL);
    add_poi_to_location(chisa_home, "stool_by_window", get_string_by_id(MAP_POI_CHISA_HOME_STOOL_NAME), get_string_by_id(MAP_POI_CHISA_HOME_STOOL_DESC), NULL);
    
    // Combined description for "书柜" and its sub_items
    add_poi_to_location(chisa_home, "bookshelf", get_string_by_id(MAP_POI_CHISA_HOME_BOOKSHELF_NAME), get_string_by_id(MAP_POI_CHISA_HOME_BOOKSHELF_DESC), NULL);
    
    // Combined description for "床" and its sub_items
    add_poi_to_location(chisa_home, "bed", get_string_by_id(MAP_POI_CHISA_HOME_BED_NAME), get_string_by_id(MAP_POI_CHISA_HOME_BED_DESC), NULL);
    
    add_poi_to_location(chisa_home, "empty_tripod", get_string_by_id(MAP_POI_CHISA_HOME_TRIPOD_NAME), get_string_by_id(MAP_POI_CHISA_HOME_TRIPOD_DESC), NULL);
    
    return CHISA_HOME_ROOM_COUNT;
}
