#ifndef CYBERIA_CLUB_LAYOUT_H
#define CYBERIA_CLUB_LAYOUT_H

#include "game_types.h"

#define CYBERIA_CLUB_ROOM_COUNT 1

/**
 * @brief Populates a given array with the Location data for the Cyberia Club.
 * 
 * @param all_locations A pointer to an array of Location structs to be filled. 
 * @param starting_index The index in the main game's location array where this room should be added.
 * @return The number of locations actually created.
 */
int create_cyberia_club_layout(Location* all_locations, int starting_index);

#endif // CYBERIA_CLUB_LAYOUT_H
