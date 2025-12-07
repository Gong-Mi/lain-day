#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "string_table.h"
#include "string_table.h"
#include "conditions.h"
#include "scenes.h" // For SCENE_MIKA_ROOM_LOCKED etc.
#include "flag_system.h" // For hash_table_get
#include "characters/mika.h"

// Helper function to safely add a connection to a location
static void add_connection(Location* loc, const char* action_id, const char* target_id, is_accessible_func is_accessible, const char* access_denied_scene_id, const char* target_scene_id) {
    if (loc == NULL || target_id == NULL || action_id == NULL) return;
    if (loc->connection_count < MAX_CONNECTIONS) {
        Connection* conn = &loc->connections[loc->connection_count];
        conn->target_location_id = target_id;
        conn->action_id = action_id;
        conn->is_accessible = is_accessible;
        conn->access_denied_scene_id = access_denied_scene_id;
        conn->target_scene_id = target_scene_id; // Store the new scene ID
        loc->connection_count++;
    } else {
        fprintf(stderr, "Warning: Max connections reached for location %s. Cannot add %s.\n", loc->id, target_id);
    }
}

// Helper function to safely add a POI to a location
static void add_poi(Location* loc, const char* poi_id, const char* poi_name, const char* poi_desc, const char* view_scene_id, const char* examine_action_id) {
    if (loc == NULL || poi_id == NULL) return;
    if (loc->pois_count < MAX_POIS) {
        POI* new_poi = &loc->pois[loc->pois_count];
        strncpy(new_poi->id, poi_id, MAX_NAME_LENGTH - 1);
        new_poi->id[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->name, poi_name, MAX_NAME_LENGTH - 1);
        new_poi->name[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->description, poi_desc, (MAX_DESC_LENGTH * 2) - 1); // MAX_DESC_LENGTH * 2 for POIs
        new_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';
        new_poi->view_scene_id = view_scene_id;
        new_poi->examine_action_id = examine_action_id;
        loc->pois_count++;
    } else {
        fprintf(stderr, "Warning: Max POIs reached for location %s. Cannot add %s.\n", loc->id, poi_id);
    }
}

int create_iwakura_house_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    // Define the 8 rooms
    Location* front_yard = &all_locations[starting_index + 0];
    Location* lower_hallway = &all_locations[starting_index + 1];
    Location* living_dining_kitchen = &all_locations[starting_index + 2];
    Location* bathroom = &all_locations[starting_index + 3];
    Location* upper_hallway = &all_locations[starting_index + 4];
    Location* lains_room = &all_locations[starting_index + 5];
    Location* mikas_room = &all_locations[starting_index + 6]; // New room
    Location* study = &all_locations[starting_index + 7];      // New room

    // --- 1. Front Yard (前院) ---
    *front_yard = (Location){0}; // Zero out the struct
    strcpy(front_yard->id, "iwakura_front_yard");
    strcpy(front_yard->name, get_string_by_id(MAP_LOCATION_FRONT_YARD_NAME));
    strcpy(front_yard->description, get_string_by_id(MAP_LOCATION_FRONT_YARD_DESC));
    add_connection(front_yard, "house", "iwakura_lower_hallway", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_poi(front_yard, "mailbox", get_string_by_id(MAP_POI_FRONT_YARD_MAILBOX_NAME), get_string_by_id(MAP_POI_FRONT_YARD_MAILBOX_DESC), NULL, NULL);
    add_poi(front_yard, "doorbell", get_string_by_id(MAP_POI_FRONT_YARD_DOORBELL_NAME), get_string_by_id(MAP_POI_FRONT_YARD_DOORBELL_DESC), NULL, NULL);

    // --- 2. Lower Hallway (下走廊) ---
    *lower_hallway = (Location){0};
    strcpy(lower_hallway->id, "iwakura_lower_hallway");
    strcpy(lower_hallway->name, get_string_by_id(MAP_LOCATION_LOWER_HALLWAY_NAME));
    strcpy(lower_hallway->description, get_string_by_id(MAP_LOCATION_LOWER_HALLWAY_DESC));
    add_connection(lower_hallway, "outside", "iwakura_front_yard", NULL, NULL, "SCENE_00_ENTRY");
    add_connection(lower_hallway, "living_area", "iwakura_living_dining_kitchen", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_connection(lower_hallway, "bathroom", "iwakura_bathroom", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_connection(lower_hallway, "upstairs", "iwakura_upper_hallway", NULL, NULL, "SCENE_IWAKURA_UPPER_HALLWAY");
    add_connection(lower_hallway, "study", "iwakura_study", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_poi(lower_hallway, "shoe_rack", get_string_by_id(MAP_POI_LOWER_HALLWAY_SHOE_RACK_NAME), get_string_by_id(MAP_POI_LOWER_HALLWAY_SHOE_RACK_DESC), NULL, NULL);
    add_poi(lower_hallway, "telephone", get_string_by_id(MAP_POI_LOWER_HALLWAY_TELEPHONE_NAME), get_string_by_id(MAP_POI_LOWER_HALLWAY_TELEPHONE_DESC), NULL, NULL);
    add_poi(lower_hallway, "umbrella_stand", get_string_by_id(MAP_POI_LOWER_HALLWAY_UMBRELLA_STAND_NAME), get_string_by_id(MAP_POI_LOWER_HALLWAY_UMBRELLA_STAND_DESC), NULL, NULL);

    // --- 3. Living-Dining-Kitchen (客厅-餐厅-厨房) ---
    *living_dining_kitchen = (Location){0};
    strcpy(living_dining_kitchen->id, "iwakura_living_dining_kitchen");
    strcpy(living_dining_kitchen->name, get_string_by_id(MAP_LOCATION_LIVING_DINING_KITCHEN_NAME));
    strcpy(living_dining_kitchen->description, get_string_by_id(MAP_LOCATION_LIVING_DINING_KITCHEN_DESC));
    add_connection(living_dining_kitchen, "hallway", "iwakura_lower_hallway", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_poi(living_dining_kitchen, "sofa", get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_SOFA_NAME), get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_SOFA_DESC), NULL, NULL);
    add_poi(living_dining_kitchen, "tv", get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_TV_NAME), get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_TV_DESC), NULL, NULL);
    add_poi(living_dining_kitchen, "dining_table", get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_DINING_TABLE_NAME), get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_DINING_TABLE_DESC), NULL, NULL);
    add_poi(living_dining_kitchen, "refrigerator", get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_REFRIGERATOR_NAME), get_string_by_id(MAP_POI_LIVING_DINING_KITCHEN_REFRIGERATOR_DESC), "SCENE_EXAMINE_FRIDGE", NULL);

    // --- 4. Bathroom (浴室) ---
    *bathroom = (Location){0};
    strcpy(bathroom->id, "iwakura_bathroom");
    strcpy(bathroom->name, get_string_by_id(MAP_LOCATION_BATHROOM_NAME));
    strcpy(bathroom->description, get_string_by_id(MAP_LOCATION_BATHROOM_DESC));
    add_connection(bathroom, "hallway", "iwakura_lower_hallway", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_poi(bathroom, "sink", get_string_by_id(MAP_POI_BATHROOM_SINK_NAME), get_string_by_id(MAP_POI_BATHROOM_SINK_DESC), NULL, NULL);
    add_poi(bathroom, "bathtub", get_string_by_id(MAP_POI_BATHROOM_BATHTUB_NAME), get_string_by_id(MAP_POI_BATHROOM_BATHTUB_DESC), NULL, NULL);

    // --- 5. Upper Hallway (上走廊) ---
    *upper_hallway = (Location){0};
    strcpy(upper_hallway->id, "iwakura_upper_hallway");
    strcpy(upper_hallway->name, get_string_by_id(MAP_LOCATION_UPPER_HALLWAY_NAME));
    strcpy(upper_hallway->description, get_string_by_id(MAP_LOCATION_UPPER_HALLWAY_DESC));
    add_connection(upper_hallway, "downstairs", "iwakura_lower_hallway", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_connection(upper_hallway, "lains_room", "iwakura_lains_room", NULL, NULL, "SCENE_01_LAIN_ROOM");
    add_connection(upper_hallway, "mikas_room", "iwakura_mikas_room", get_mika_module()->is_room_accessible, "SCENE_MIKA_ROOM_LOCKED", "SCENE_MIKA_ROOM_UNLOCKED");
    add_poi(upper_hallway, "painting", get_string_by_id(MAP_POI_UPPER_HALLWAY_PAINTING_NAME), get_string_by_id(MAP_POI_UPPER_HALLWAY_PAINTING_DESC), NULL, NULL);

    // --- 6. Lain's Room (Lain的房间) ---
    *lains_room = (Location){0};
    strcpy(lains_room->id, "iwakura_lains_room");
    strcpy(lains_room->name, get_string_by_id(MAP_LOCATION_LAINS_ROOM_NAME_IWAKURA));
    strcpy(lains_room->description, get_string_by_id(MAP_LOCATION_LAINS_ROOM_DESC_IWAKURA));
    add_connection(lains_room, "upper_hallway", "iwakura_upper_hallway", NULL, NULL, "SCENE_IWAKURA_UPPER_HALLWAY");
    // POIs from original lain_room
    add_poi(lains_room, "navi_computer", get_string_by_id(MAP_POI_LAINS_ROOM_NAVI_COMPUTER_NAME), get_string_by_id(MAP_POI_LAINS_ROOM_NAVI_COMPUTER_DESC), NULL, "examine_navi");
    add_poi(lains_room, "bed", get_string_by_id(MAP_POI_LAINS_ROOM_BED_NAME_IWAKURA), get_string_by_id(MAP_POI_LAINS_ROOM_BED_DESC_IWAKURA), NULL, NULL);
    add_poi(lains_room, "window", get_string_by_id(MAP_POI_LAINS_ROOM_WINDOW_NAME), get_string_by_id(MAP_POI_LAINS_ROOM_WINDOW_DESC), NULL, NULL);
    add_poi(lains_room, "toy_dog", get_string_by_id(MAP_POI_LAINS_ROOM_TOY_DOG_NAME), get_string_by_id(MAP_POI_LAINS_ROOM_TOY_DOG_DESC), NULL, NULL);
    add_poi(lains_room, "bookshelf", get_string_by_id(MAP_POI_LAINS_ROOM_BOOKSHELF_NAME_IWAKURA), get_string_by_id(MAP_POI_LAINS_ROOM_BOOKSHELF_DESC_IWAKURA), NULL, NULL);

    // --- 7. Mika's Room (美香的房间) ---
    *mikas_room = (Location){0};
    strcpy(mikas_room->id, "iwakura_mikas_room");
    strcpy(mikas_room->name, get_string_by_id(MAP_LOCATION_MIKAS_ROOM_NAME));
    strcpy(mikas_room->description, get_string_by_id(MAP_LOCATION_MIKAS_ROOM_DESC));
    add_connection(mikas_room, "upper_hallway", "iwakura_upper_hallway", NULL, NULL, "SCENE_IWAKURA_UPPER_HALLWAY");
    add_poi(mikas_room, "desk", get_string_by_id(MAP_POI_MIKAS_ROOM_DESK_NAME), get_string_by_id(MAP_POI_MIKAS_ROOM_DESK_DESC), NULL, NULL);
    add_poi(mikas_room, "wardrobe", get_string_by_id(MAP_POI_MIKAS_ROOM_WARDROBE_NAME), get_string_by_id(MAP_POI_MIKAS_ROOM_WARDROBE_DESC), NULL, NULL);
    
    // --- 8. Study (书房) ---
    *study = (Location){0};
    strcpy(study->id, "iwakura_study");
    strcpy(study->name, get_string_by_id(MAP_LOCATION_STUDY_NAME));
    strcpy(study->description, get_string_by_id(MAP_LOCATION_STUDY_DESC));
    add_connection(study, "hallway", "iwakura_lower_hallway", NULL, NULL, "SCENE_02_DOWNSTAIRS");
    add_poi(study, "bookshelf", get_string_by_id(MAP_POI_STUDY_BOOKSHELF_NAME), get_string_by_id(MAP_POI_STUDY_BOOKSHELF_DESC), NULL, NULL);
    add_poi(study, "desk", get_string_by_id(MAP_POI_STUDY_DESK_NAME), get_string_by_id(MAP_POI_STUDY_DESK_DESC), NULL, NULL);
    
    return IWAKURA_HOUSE_ROOM_COUNT;
}
