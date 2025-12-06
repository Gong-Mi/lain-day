#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "game_types.h" // For GameState and other game types
#include "game_paths.h" // For GamePaths struct

// Loads the player state from the character.json file.
// Returns 1 on success, 0 on failure.
int load_player_state(const char* path, GameState* game_state);

// Loads all item data from the items.json file.
// Returns 1 on success, 0 on failure.
int load_items_data(const char* path, GameState* game_state);

// Loads the string table from strings.json.
// Returns 1 on success, 0 on failure.
int load_string_table(const char* path);

// Loads all action data from the actions.json file.
// Returns 1 on success, 0 on failure.
int load_actions_data(const char* path, GameState* game_state);

// Saves the game state to the character.json file.
// Returns 1 on success, 0 on failure.
int save_game_state(const char* path, const GameState* game_state);

// Cleans up dynamically allocated memory within the GameState.
void cleanup_game_state(GameState* game_state);

#endif // DATA_LOADER_H
