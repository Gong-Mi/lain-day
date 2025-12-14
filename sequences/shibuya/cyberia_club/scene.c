#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "conditions.h" // For potential future accessibility functions
#include "flag_system.h" // For potential future flag checks
#include "game_types.h" // For MAX_NAME_LENGTH, MAX_DESC_LENGTH etc.
#include "map_loader.h" // For global helper functions
#include "string_table.h" // For get_string_by_id

int create_cyberia_club_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* cyberia_club = &all_locations[starting_index];
    *cyberia_club = (Location){0}; // Zero out the struct

    strcpy(cyberia_club->id, "cyberia_club");
    strcpy(cyberia_club->name, get_string_by_id(MAP_LOCATION_CYBERIA_CLUB_NAME));
    strcpy(cyberia_club->description, get_string_by_id(MAP_LOCATION_CYBERIA_CLUB_DESC));

    // Connections (from old_map/cyberia_club/connections.json - which was empty)
    // Add a default connection to Shibuya Street for now
    add_connection_to_location(cyberia_club, "exit_club", "shibuya_street", NULL, NULL, NULL);

    // POIs from old_map/cyberia_club/poi.json
    add_poi_to_location(cyberia_club, "dance_floor", get_string_by_id(MAP_POI_CYBERIA_CLUB_DANCE_FLOOR_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_DANCE_FLOOR_DESC), NULL);
    add_poi_to_location(cyberia_club, "bar", get_string_by_id(MAP_POI_CYBERIA_CLUB_BAR_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_BAR_DESC), NULL);
    add_poi_to_location(cyberia_club, "dj", get_string_by_id(MAP_POI_CYBERIA_CLUB_DJ_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_DJ_DESC), NULL);
    add_poi_to_location(cyberia_club, "old_mic", get_string_by_id(MAP_POI_CYBERIA_CLUB_OLD_MIC_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_OLD_MIC_DESC), "examine_old_mic"); // Action: examine_old_mic
    add_poi_to_location(cyberia_club, "detective_kids", get_string_by_id(MAP_POI_CYBERIA_CLUB_DETECTIVE_KIDS_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_DETECTIVE_KIDS_DESC), NULL);
    add_poi_to_location(cyberia_club, "restroom", get_string_by_id(MAP_POI_CYBERIA_CLUB_RESTROOM_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_RESTROOM_DESC), NULL);
    add_poi_to_location(cyberia_club, "boss", get_string_by_id(MAP_POI_CYBERIA_CLUB_BOSS_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_BOSS_DESC), NULL); // Sells: milk, coffee, juice
    add_poi_to_location(cyberia_club, "business_card", get_string_by_id(MAP_POI_CYBERIA_CLUB_BUSINESS_CARD_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_BUSINESS_CARD_DESC), NULL);
    add_poi_to_location(cyberia_club, "weird_youth", get_string_by_id(MAP_POI_CYBERIA_CLUB_WEIRD_YOUTH_NAME), get_string_by_id(MAP_POI_CYBERIA_CLUB_WEIRD_YOUTH_DESC), NULL);
    
    return CYBERIA_CLUB_ROOM_COUNT;
}
