#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "game_types.h"

// Loads all map data from the specified directory into the GameState.
// Returns 1 on success, 0 on failure.
int load_map_data(const char* map_dir_path, GameState* game_state);

// Helper functions for programmatic map definition
void init_location(Location* loc, const char* id, const char* name, const char* description);
void add_poi_to_location(Location* loc, const char* id, const char* name, const char* description);
void add_connection_to_location(Location* loc, const char* target_id);

#endif // MAP_LOADER_H
