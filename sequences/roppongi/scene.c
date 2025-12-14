#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "map_loader.h" // For helper functions

int create_roppongi_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* roppongi_street = &all_locations[starting_index];
    Location* school_gate = &all_locations[starting_index + 1];
    Location* school_hallway = &all_locations[starting_index + 2];
    Location* classroom = &all_locations[starting_index + 3];
    Location* rooftop = &all_locations[starting_index + 4];

    // --- Roppongi Street ---
    init_location(roppongi_street, "roppongi_street", get_string_by_id(MAP_LOCATION_ROPPONGI_STREET_NAME), get_string_by_id(MAP_LOCATION_ROPPONGI_STREET_DESC));
    // Connection to Ebisu Station (Train System) - assuming "ebisu" is the ID from station_coordinates.json
    add_connection_to_location(roppongi_street, "go_to_station", "ebisu", NULL, NULL, NULL); 
    add_connection_to_location(roppongi_street, "go_to_school", "roppongi_school_gate", NULL, NULL, NULL);

    // --- School Gate ---
    init_location(school_gate, "roppongi_school_gate", get_string_by_id(MAP_LOCATION_ROPPONGI_SCHOOL_GATE_NAME), get_string_by_id(MAP_LOCATION_ROPPONGI_SCHOOL_GATE_DESC));
    add_connection_to_location(school_gate, "enter_school", "roppongi_school_hallway", NULL, NULL, NULL);
    add_connection_to_location(school_gate, "leave_school", "roppongi_street", NULL, NULL, NULL);

    // --- School Hallway ---
    init_location(school_hallway, "roppongi_school_hallway", get_string_by_id(MAP_LOCATION_ROPPONGI_SCHOOL_HALLWAY_NAME), get_string_by_id(MAP_LOCATION_ROPPONGI_SCHOOL_HALLWAY_DESC));
    add_connection_to_location(school_hallway, "enter_classroom", "roppongi_classroom", NULL, NULL, NULL); // Needs scene SCENE_07_CLASSROOM?
    add_connection_to_location(school_hallway, "go_to_rooftop", "roppongi_school_rooftop", NULL, NULL, NULL);
    add_connection_to_location(school_hallway, "exit_building", "roppongi_school_gate", NULL, NULL, NULL);

    // --- Classroom ---
    init_location(classroom, "roppongi_classroom", get_string_by_id(MAP_LOCATION_ROPPONGI_CLASSROOM_NAME), get_string_by_id(MAP_LOCATION_ROPPONGI_CLASSROOM_DESC));
    add_connection_to_location(classroom, "leave_classroom", "roppongi_school_hallway", NULL, NULL, NULL);
    add_poi_to_location(classroom, "my_desk", get_string_by_id(MAP_POI_ROPPONGI_CLASSROOM_DESK_NAME), get_string_by_id(MAP_POI_ROPPONGI_CLASSROOM_DESK_DESC), NULL);
    add_poi_to_location(classroom, "blackboard", get_string_by_id(MAP_POI_ROPPONGI_CLASSROOM_BLACKBOARD_NAME), get_string_by_id(MAP_POI_ROPPONGI_CLASSROOM_BLACKBOARD_DESC), NULL);

    // --- Rooftop ---
    init_location(rooftop, "roppongi_school_rooftop", get_string_by_id(MAP_LOCATION_ROPPONGI_ROOFTOP_NAME), get_string_by_id(MAP_LOCATION_ROPPONGI_ROOFTOP_DESC));
    add_connection_to_location(rooftop, "go_downstairs", "roppongi_school_hallway", NULL, NULL, NULL);
    add_poi_to_location(rooftop, "fence", get_string_by_id(MAP_POI_ROPPONGI_ROOFTOP_FENCE_NAME), get_string_by_id(MAP_POI_ROPPONGI_ROOFTOP_FENCE_DESC), NULL);

    return ROPPONGI_LAYOUT_ROOM_COUNT;
}
