#ifndef GAME_PATHS_H
#define GAME_PATHS_H

#include "game_types.h" // For MAX_PATH_LENGTH

// --- Path Management Struct ---
typedef struct {
    char base_path[MAX_PATH_LENGTH];
    char items_file[MAX_PATH_LENGTH];
    char actions_file[MAX_PATH_LENGTH];
    char map_dir[MAX_PATH_LENGTH];
    char session_root_dir[MAX_PATH_LENGTH];
} GamePaths;

#endif // GAME_PATHS_H
