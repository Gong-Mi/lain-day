#ifndef IWAKURA_LAYOUT_H
#define IWAKURA_LAYOUT_H

#include "game_types.h"

#define IWAKURA_HOUSE_ROOM_COUNT 8

/**
 * @brief Populates a given array with the Location data for the Iwakura house.
 * 
 * This function programmatically defines the rooms of the Iwakura house,
 * their properties (name, description), and their connections to each other.
 * It is designed to be called to dynamically load this complex location into
 * the game's location map.
 * 
 * @param house_locations A pointer to an array of Location structs to be filled. 
 *                        The array must be large enough to hold IWAKURA_HOUSE_ROOM_COUNT locations.
 * @param starting_index The index in the main game's location array where these rooms should be added.
 * @return The number of locations actually created.
 */
int create_iwakura_house_layout(Location* all_locations, int starting_index);

#endif // IWAKURA_LAYOUT_H
