#include "map_loader.h"
#include "string_table.h"
#include "cJSON.h" // Still needed for other uses potentially, keep for now
#include "cmap.h" // Include our new CMap header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <dirent.h> // No longer needed for file system traversal
// #include <sys/stat.h> // No longer needed for file system checks
// #include <ctype.h> // No longer directly used for map loading

#include "../sequences/miyanosaka/iwakura_house/scene.h" // For dynamic layout
#include "../sequences/shibuya/cyberia_club/scene.h" // For dynamic layout
#include "../sequences/shinjuku/chisa_home/scene.h" // For dynamic layout

// --- Helper Functions for Programmatic Map Definition ---

void init_location(Location* loc, const char* id, const char* name, const char* description) {
    memset(loc, 0, sizeof(Location));
    strncpy(loc->id, id, MAX_NAME_LENGTH - 1);
    loc->id[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(loc->name, name, MAX_NAME_LENGTH - 1);
    loc->name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(loc->description, description, (MAX_DESC_LENGTH * 4) - 1);
    loc->description[(MAX_DESC_LENGTH * 4) - 1] = '\0';
}

void add_poi_to_location(Location* loc, const char* id, const char* name, const char* description) {
    if (loc->pois_count >= MAX_POIS) {
        fprintf(stderr, "WARNING: Max POIs reached for location %s. Cannot add %s.\n", loc->id, id);
        return;
    }
    POI* current_poi = &loc->pois[loc->pois_count];
    memset(current_poi, 0, sizeof(POI));
    strncpy(current_poi->id, id, MAX_NAME_LENGTH - 1);
    current_poi->id[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(current_poi->name, name, MAX_NAME_LENGTH - 1);
    current_poi->name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(current_poi->description, description, (MAX_DESC_LENGTH * 2) - 1);
    current_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';
    loc->pois_count++;
}

void add_connection_to_location(Location* loc, const char* action_id, const char* target_location_id, is_accessible_func is_accessible, const char* access_denied_scene_id) {
    if (loc->connection_count >= MAX_CONNECTIONS) {
        fprintf(stderr, "WARNING: Max connections reached for location %s. Cannot add connection to %s.\n", loc->id, target_location_id);
        return;
    }
    Connection* conn = &loc->connections[loc->connection_count];
    conn->action_id = action_id;
    conn->target_location_id = target_location_id;
    conn->is_accessible = is_accessible;
    conn->access_denied_scene_id = access_denied_scene_id;
    loc->connection_count++;
}

// --- Programmatic Map Data Definition ---

static int load_programmatic_map_data(GameState* game_state) {
    if (game_state->location_count >= MAX_LOCATIONS) {
        fprintf(stderr, "WARNING: Max locations reached. Cannot add more programmatic locations.\n");
        return 0;
    }

    // --- Placeholder: lain_room ---
    Location* lain_room = &game_state->all_locations[game_state->location_count];
    init_location(lain_room, "lain_room", get_string_by_id(MAP_LOCATION_LAIN_ROOM_NAME), get_string_by_id(MAP_LOCATION_LAIN_ROOM_DESC));
    add_poi_to_location(lain_room, "navi", get_string_by_id(MAP_POI_LAIN_ROOM_NAVI_NAME), get_string_by_id(MAP_POI_LAIN_ROOM_NAVI_DESC));
    add_poi_to_location(lain_room, "bed", get_string_by_id(MAP_POI_LAIN_ROOM_BED_NAME), get_string_by_id(MAP_POI_LAIN_ROOM_BED_DESC));
    add_connection_to_location(lain_room, "go_downstairs", "downstairs", NULL, NULL);
    cmap_insert(game_state->location_map, lain_room);
#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Programmatically added location: %s\n", lain_room->id);
#endif
    game_state->location_count++;

    // --- Placeholder: downstairs ---
    if (game_state->location_count >= MAX_LOCATIONS) return 0;
    Location* downstairs = &game_state->all_locations[game_state->location_count];
    init_location(downstairs, "downstairs", get_string_by_id(MAP_LOCATION_DOWNSTAIRS_NAME), get_string_by_id(MAP_LOCATION_DOWNSTAIRS_DESC));
    add_poi_to_location(downstairs, "kitchen", get_string_by_id(MAP_POI_DOWNSTAIRS_KITCHEN_NAME), get_string_by_id(MAP_POI_DOWNSTAIRS_KITCHEN_DESC));
    add_poi_to_location(downstairs, "tv", get_string_by_id(MAP_POI_DOWNSTAIRS_TV_NAME), get_string_by_id(MAP_POI_DOWNSTAIRS_TV_DESC));
    add_connection_to_location(downstairs, "enter_lain_room", "lain_room", NULL, NULL);
    add_connection_to_location(downstairs, "go_outside", "outside_house", NULL, NULL);
    cmap_insert(game_state->location_map, downstairs);
#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Programmatically added location: %s\n", downstairs->id);
#endif
    game_state->location_count++;

    // --- Placeholder: outside_house ---
    if (game_state->location_count >= MAX_LOCATIONS) return 0;
    Location* outside_house = &game_state->all_locations[game_state->location_count];
    init_location(outside_house, "outside_house", get_string_by_id(MAP_LOCATION_OUTSIDE_HOUSE_NAME), get_string_by_id(MAP_LOCATION_OUTSIDE_HOUSE_DESC));
    add_poi_to_location(outside_house, "mailbox", get_string_by_id(MAP_POI_OUTSIDE_HOUSE_MAILBOX_NAME), get_string_by_id(MAP_POI_OUTSIDE_HOUSE_MAILBOX_DESC));
    add_connection_to_location(outside_house, "go_downstairs", "downstairs", NULL, NULL); // Action leads back
    add_connection_to_location(outside_house, "go_to_shibuya", "shibuya_street", NULL, NULL);
    cmap_insert(game_state->location_map, outside_house);
#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Programmatically added location: %s\n", outside_house->id);
#endif
    game_state->location_count++;

    // --- Placeholder: shibuya_street ---
    if (game_state->location_count >= MAX_LOCATIONS) return 0;
    Location* shibuya_street = &game_state->all_locations[game_state->location_count];
    init_location(shibuya_street, "shibuya_street", get_string_by_id(MAP_LOCATION_SHIBUYA_STREET_NAME), get_string_by_id(MAP_LOCATION_SHIBUYA_STREET_DESC));
    add_poi_to_location(shibuya_street, "crosswalk", get_string_by_id(MAP_POI_SHIBUYA_STREET_CROSSWALK_NAME), get_string_by_id(MAP_POI_SHIBUYA_STREET_CROSSWALK_DESC));
    add_connection_to_location(shibuya_street, "go_home", "outside_house", NULL, NULL);
    add_connection_to_location(shibuya_street, "enter_cyberia", "cyberia_club", NULL, NULL);
    cmap_insert(game_state->location_map, shibuya_street);
#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Programmatically added location: %s\n", shibuya_street->id);
#endif
    game_state->location_count++;


    return 1; // Success
}

// --- Public API Implementation ---

int load_map_data(const char* map_dir_path, GameState* game_state) {
#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Entering load_map_data (programmatic) for path: %s\n", map_dir_path); // Path is ignored now
#endif

    if (game_state == NULL) {
        fprintf(stderr, "ERROR: GameState is NULL in load_map_data.\n");
        return 0;
    }

    // --- CMap Integration: Create the hash map ---
    game_state->location_map = cmap_create(MAX_LOCATIONS);
    if (game_state->location_map == NULL) {
        fprintf(stderr, "ERROR: Failed to create location map hash table.\n");
        return 0;
    }

#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: CMap created with size %d.\n", MAX_LOCATIONS);
#endif

    // Load locations programmatically
    if (!load_programmatic_map_data(game_state)) {
        fprintf(stderr, "ERROR: Failed to load programmatic map data.\n");
        cmap_destroy(game_state->location_map);
        return 0;
    }

    // --- Dynamically add Iwakura House layout ---
    int rooms_added = create_iwakura_house_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        // Need to insert newly added locations into the cmap as well.
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[game_state->location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        game_state->location_count += rooms_added;

        // DEBUG: Verify POI counts for newly added locations
#ifdef USE_MAP_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Verifying POIs for dynamically added locations:\n");
        for (int i = 0; i < rooms_added; ++i) {
            Location* loc = &game_state->all_locations[game_state->location_count - rooms_added + i];
            fprintf(stderr, "DEBUG:   Location '%s' (pois_count: %d)\n", loc->id, loc->pois_count);
            if (loc->pois_count > 0) {
                fprintf(stderr, "DEBUG:     First POI: '%s' ('%s')\n", loc->pois[0].id, loc->pois[0].name);
            }
        }
#endif
    }

    // --- Dynamically add Cyberia Club layout ---
    rooms_added = create_cyberia_club_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[game_state->location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        game_state->location_count += rooms_added;

#ifdef USE_MAP_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Verifying POIs for dynamically added Cyberia Club locations:\n");
        for (int i = 0; i < rooms_added; ++i) {
            Location* loc = &game_state->all_locations[game_state->location_count - rooms_added + i];
            fprintf(stderr, "DEBUG:   Location '%s' (pois_count: %d)\n", loc->id, loc->pois_count);
            if (loc->pois_count > 0) {
                fprintf(stderr, "DEBUG:     First POI: '%s' ('%s')\n", loc->pois[0].id, loc->pois[0].name);
            }
        }
#endif
    }

    // --- Dynamically add Chisa Home layout ---
    rooms_added = create_chisa_home_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[game_state->location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        game_state->location_count += rooms_added;

#ifdef USE_MAP_DEBUG_LOGGING
        fprintf(stderr, "DEBUG: Verifying POIs for dynamically added Chisa Home locations:\n");
        for (int i = 0; i < rooms_added; ++i) {
            Location* loc = &game_state->all_locations[game_state->location_count - rooms_added + i];
            fprintf(stderr, "DEBUG:   Location '%s' (pois_count: %d)\n", loc->id, loc->pois_count);
            if (loc->pois_count > 0) {
                fprintf(stderr, "DEBUG:     First POI: '%s' ('%s')\n", loc->pois[0].id, loc->pois[0].name);
            }
        }
#endif
    }


#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Successfully loaded %d locations (programmatic + dynamic) .\n", game_state->location_count);
#endif

    return 1;
}