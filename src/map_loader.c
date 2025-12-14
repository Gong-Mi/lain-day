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
#include "../sequences/miyanosaka/street/scene.h" // For dynamic layout
#include "../sequences/miyanosaka/station/scene.h" // For dynamic layout
#include "../sequences/shibuya/cyberia_club/scene.h" // For dynamic layout
#include "../sequences/shinjuku/chisa_home/scene.h" // For dynamic layout
#include "../sequences/shinjuku/scene.h" // For dynamic layout in Shinjuku area
#include "../sequences/train_station/scene.h" // For dynamic layout

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

void add_poi_to_location(Location* loc, const char* id, const char* name, const char* description, const char* examine_action_id) {
    if (loc->pois_count >= MAX_POIS) {
        fprintf(stderr, "WARNING: Max POIs reached for location %s. Cannot add %s.\n", loc->id, id);
        return;
    }
    POI* poi = &loc->pois[loc->pois_count];
    strncpy(poi->id, id, MAX_NAME_LENGTH - 1);
    strncpy(poi->name, name, MAX_NAME_LENGTH - 1);
    strncpy(poi->description, description, MAX_DESC_LENGTH - 1);
    
    // Assign the provided examine action ID
    poi->examine_action_id = examine_action_id; 

    loc->pois_count++;
}

void add_connection_to_location(Location* loc, const char* action_id, const char* target_location_id, is_accessible_func is_accessible, const char* access_denied_scene_id, const char* target_scene_id) {
    if (loc->connection_count >= MAX_CONNECTIONS) {
        fprintf(stderr, "WARNING: Max connections reached for location %s. Cannot add connection to %s.\n", loc->id, target_location_id);
        return;
    }
    Connection* conn = &loc->connections[loc->connection_count];
    conn->action_id = action_id;
    conn->target_location_id = target_location_id;
    conn->is_accessible = is_accessible;
    conn->access_denied_scene_id = access_denied_scene_id;
    conn->target_scene_id = target_scene_id; // Store the new scene ID
    loc->connection_count++;
}

// --- Programmatic Map Data Definition ---

static int load_programmatic_map_data(GameState* game_state) {
    if (game_state->location_count >= MAX_LOCATIONS) {
        fprintf(stderr, "WARNING: Max locations reached. Cannot add more programmatic locations.\n");
        return 0;
    }

    // All programmatic locations are now deprecated and have been moved to
    // dynamic layout creation in the /sequences directory.
    // This function remains as a stub.

    return 1; // Success
}

// --- Public API Implementation ---

// Helper function to get a location by its ID from the GameState's map
Location* get_location_by_id(const char* location_id) {
    if (game_state == NULL || game_state->location_map == NULL || location_id == NULL) {
        return NULL;
    }
    return (Location*)cmap_get(game_state->location_map, location_id);
}

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
    int prev_location_count = game_state->location_count; 
    int rooms_added = create_iwakura_house_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        // Update total count *before* inserting to cmap, so new_loc points to correct place
        game_state->location_count += rooms_added; 

        // Need to insert newly added locations into the cmap as well.
        for (int i = 0; i < rooms_added; ++i) {
            // new_loc should point to the location that was just added starting from prev_location_count
            Location* new_loc = &game_state->all_locations[prev_location_count + i]; 
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
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
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_cyberia_club_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        // ... (debug code) ...
    }

    // --- Dynamically add Chisa Home layout ---
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_chisa_home_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        // ... (debug code) ...
    }

    // --- Dynamically add Shinjuku layout ---
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_shinjuku_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
#ifdef USE_MAP_DEBUG_LOGGING
            fprintf(stderr, "DEBUG: Inserted dynamic location '%s' into CMap.\n", new_loc->id);
#endif
        }
        // ... (debug code) ...
    }

    // --- Dynamically add Miyanosaka Street layout ---
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_miyanosaka_street_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
        }
    }

    // --- Dynamically add Miyanosaka Station layout ---
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_miyanosaka_station_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
        }
    }

    // --- Dynamically add Train Station layout ---
    prev_location_count = game_state->location_count; // Capture current count
    rooms_added = create_train_station_layout(game_state->all_locations, game_state->location_count);
    if (rooms_added > 0) {
        game_state->location_count += rooms_added; // Update total count
        for (int i = 0; i < rooms_added; ++i) {
            Location* new_loc = &game_state->all_locations[prev_location_count + i];
            cmap_insert(game_state->location_map, new_loc);
        }
    }


#ifdef USE_MAP_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Successfully loaded %d locations (programmatic + dynamic) .\n", game_state->location_count);
#endif

    return 1;
}