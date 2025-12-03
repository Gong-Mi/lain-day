#ifndef CHISA_HOME_LAYOUT_H
#define CHISA_HOME_LAYOUT_H

#include "game_types.h"

#define CHISA_HOME_ROOM_COUNT 1

/**
 * @brief Populates a given array with the Location data for Chisa's Home.
 * 
 * @param all_locations A pointer to an array of Location structs to be filled. 
 * @param starting_index The index in the main game's location array where this room should be added.
 * @return The number of locations actually created.
 */
int create_chisa_home_layout(Location* all_locations, int starting_index);

#endif // CHISA_HOME_LAYOUT_H
