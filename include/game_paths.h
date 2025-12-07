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

void init_paths(char* argv0, GamePaths* paths);
void get_base_path(char* exe_path, char* base_path, size_t size);
int copy_file(const char *src_path, const char *dest_path);
int write_string_to_file(const char* str, const char* dest_path);

// Ensures a directory exists, creating parent directories as needed.
// Returns true on success, false on failure (e.g., permissions).
bool ensure_directory_exists_recursive(const char* path, mode_t mode);

#endif // GAME_PATHS_H
