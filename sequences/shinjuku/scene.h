#ifndef SHINJUKU_LAYOUT_H
#define SHINJUKU_LAYOUT_H

#include "game_types.h"

// Define the number of locations in Shinjuku layout
#define SHINJUKU_LAYOUT_COUNT 2 // Abandoned site and potentially Shinjuku Station area

/**
 * @brief Populates a given array with the Location data for the Shinjuku area.
 * 
 * This function programmatically defines locations in Shinjuku,
 * their properties (name, description), and their connections.
 * 
 * @param all_locations A pointer to an array of Location structs to be filled. 
 * @param starting_index The index in the main game's location array where these rooms should be added.
 * @return The number of locations actually created.
 */
int create_shinjuku_layout(Location* all_locations, int starting_index);

#endif // SHINJUKU_LAYOUT_H
