#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "game_types.h"

// Represents the possible outcomes of a data loading operation.
typedef enum {
    DATA_LOADER_SUCCESS,
    DATA_LOADER_ERROR_FILE_OPEN,
    DATA_LOADER_ERROR_FILE_STAT,
    DATA_LOADER_ERROR_MEMORY_ALLOC,
    DATA_LOADER_ERROR_FILE_READ
} DataLoaderStatus;

/**
 * @brief Reads an entire file into a dynamically allocated buffer.
 *
 * This function is safer than a simple fopen/fread loop as it provides
 * detailed error codes. The caller is responsible for freeing the
 * allocated buffer on success.
 *
 * @param path The path to the file to read.
 * @param buffer_ptr A pointer to a char*, which will be set to the address
 *                   of the newly allocated buffer containing the file content.
 * @param length_ptr A pointer to a long, which will be set to the length
 *                   of the file.
 * @return A DataLoaderStatus enum indicating the outcome of the operation.
 */
DataLoaderStatus read_entire_file(const char* path, char** buffer_ptr, long* length_ptr);

/**
 * @brief Loads the initial game state, including items and player data.
 *
 * @param game_state A pointer to the main GameState struct to be populated.
 * @return 1 on success, 0 on failure.
 */
int load_items_data(GameState* game_state);

int load_player_state(const char* path, GameState* game_state);
int save_game_state(const char* path, const GameState* game_state);
void cleanup_game_state(GameState* game_state);
int load_string_table();

#endif //DATA_LOADER_H