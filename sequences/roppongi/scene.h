#ifndef ROPPONGI_LAYOUT_H
#define ROPPONGI_LAYOUT_H

#include "game_types.h"

// Number of locations: Street, Gate, Hallway, Classroom, Rooftop
#define ROPPONGI_LAYOUT_ROOM_COUNT 5

/**
 * @brief Populates a given array with the Location data for the Roppongi area.
 * 
 * Includes the street and the private school.
 * 
 * @param all_locations A pointer to an array of Location structs to be filled. 
 * @param starting_index The index in the main game's location array where these rooms should be added.
 * @return The number of locations actually created.
 */
int create_roppongi_layout(Location* all_locations, int starting_index);

#endif // ROPPONGI_LAYOUT_H
