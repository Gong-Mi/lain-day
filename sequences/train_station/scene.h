#ifndef TRAIN_STATION_LAYOUT_H
#define TRAIN_STATION_LAYOUT_H

#include "game_types.h"

// Define a struct to hold station data from station_coordinates.json
typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    int x; // Placeholder, might not be used in current game logic
    int y; // Placeholder, might not be used in current game logic
} TrainStationData;

#define TRAIN_STATION_COUNT 30 // From old_map/shibuya_street/connections.json.
#define TRAIN_STATION_LAYOUT_COUNT 1

/**
 * @brief Populates a given array with the Location data for the Train Station.
 * 
 * This function programmatically defines the train station,
 * its properties (name, description), and its connections.
 * It also integrates station data from the old station_coordinates.json.
 * 
 * @param all_locations A pointer to an array of Location structs to be filled. 
 * @param starting_index The index in the main game's location array where this room should be added.
 * @return The number of locations actually created.
 */
int create_train_station_layout(Location* all_locations, int starting_index);

#endif // TRAIN_STATION_LAYOUT_H
