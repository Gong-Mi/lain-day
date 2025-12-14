#ifndef GAME_PATHS_H
#define GAME_PATHS_H

#include <stdbool.h> // For bool
#include <stddef.h>  // For size_t
#include <sys/types.h> // For mode_t
#include "game_types.h" // For GamePaths struct and MAX_PATH_LENGTH

// --- Path Management Struct ---
// GamePaths struct is now defined in game_types.h

void init_paths(char* argv0, GamePaths* paths);
void get_base_path(char* exe_path, char* base_path, size_t size);
int copy_file(const char *src_path, const char *dest_path);
int write_string_to_file(const char* str, const char* dest_path);

// Ensures a directory exists, creating parent directories as needed.
// Returns true on success, false on failure (e.g., permissions).
bool ensure_directory_exists_recursive(const char* path, mode_t mode);

#endif // GAME_PATHS_H
