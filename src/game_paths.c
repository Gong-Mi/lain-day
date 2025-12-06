#include "../include/game_paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h> // For dirname
#include <errno.h>  // Required for errno and strerror
#include <sys/stat.h> // For stat and S_ISDIR
#include <unistd.h> // For realpath

void get_base_path(char* exe_path, char* base_path, size_t size) {
    char* exe_dir = dirname(exe_path);
    snprintf(base_path, size, "%s/../..", exe_dir);
}

void init_paths(char* argv0, GamePaths* paths) {
    char exe_path[MAX_PATH_LENGTH];
    char* resolved_path = realpath(argv0, exe_path);
    if (resolved_path == NULL) {
        fprintf(stderr, "Error: Could not resolve executable path for '%s'. %s\n", argv0, strerror(errno));
        exit(EXIT_FAILURE);
    }

    char current_path[MAX_PATH_LENGTH];
    strncpy(current_path, dirname(resolved_path), MAX_PATH_LENGTH - 1);

    // Search upwards for a directory containing a ".git" folder
    for (int i = 0; i < 10; ++i) { // Limit search depth to 10 levels
        char git_path[MAX_PATH_LENGTH];
        snprintf(git_path, sizeof(git_path), "%s/.git", current_path);
        
        struct stat st;
        if (stat(git_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // Found the root directory
            snprintf(paths->base_path, sizeof(paths->base_path), "%s", current_path);
            snprintf(paths->items_file, sizeof(paths->items_file), "%s/items.json", paths->base_path);
            snprintf(paths->map_dir, sizeof(paths->map_dir), "%s/map", paths->base_path);
            snprintf(paths->session_root_dir, sizeof(paths->session_root_dir), "%s/session", paths->base_path);
            return;
        }

        // Move up one directory
        char temp_path[MAX_PATH_LENGTH];
        snprintf(temp_path, sizeof(temp_path), "%s/..", current_path);
        if (realpath(temp_path, current_path) == NULL) {
            break; // Could not move up further
        }
    }

    fprintf(stderr, "Error: Could not find project root directory (containing .git folder).\n");
    exit(EXIT_FAILURE);
}

int copy_file(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "rb");
    if (src == NULL) {
        fprintf(stderr, "Error: Could not open source file %s\n", src_path);
        return 0;
    }

    FILE *dest = fopen(dest_path, "wb");
    if (dest == NULL) {
        fprintf(stderr, "Error: Could not open destination file %s\n", dest_path);
        fclose(src);
        return 0;
    }

    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }

    fclose(src);
    fclose(dest);
    return 1;
}

int write_string_to_file(const char* str, const char* dest_path) {
    FILE* dest = fopen(dest_path, "wb");
    if (dest == NULL) {
        fprintf(stderr, "Error: Could not open destination file %s\n", dest_path);
        return 0;
    }
    size_t len = strlen(str);
    if (fwrite(str, 1, len, dest) != len) {
        fprintf(stderr, "Error: Failed to write entire string to %s\n", dest_path);
        fclose(dest);
        return 0;
    }
    fclose(dest);
    return 1;
}