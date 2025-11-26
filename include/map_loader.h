#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "game_types.h"

// Loads all map data from the specified directory into the GameState.
// Returns 1 on success, 0 on failure.
int load_map_data(const char* map_dir_path, GameState* game_state);

#endif // MAP_LOADER_H
