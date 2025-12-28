#include "systems/navi_shell.h"
#include "render_utils.h"
#include "game_paths.h" // For ensure_directory_exists
#include "ansi_colors.h"
#include "linenoise.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#define SHELL_PROMPT_COLOR ANSI_COLOR_GREEN
#define SHELL_PATH_COLOR ANSI_COLOR_BLUE
#define SHELL_ERROR_COLOR ANSI_COLOR_RED

// Virtual root path inside the project
#define VIRTUAL_ROOT "world"

// Maximum path length for virtual paths
#define MAX_VIRTUAL_PATH 256

typedef struct {
    char current_virtual_path[MAX_VIRTUAL_PATH]; // e.g., "/home/lain"
    char physical_root_path[MAX_PATH_LENGTH];    // Absolute path to project root/world
} ShellState;

static void get_absolute_path(const ShellState* state, const char* virtual_path, char* out_path, size_t size) {
    // Construct absolute physical path: physical_root_path + virtual_path
    // Remove leading slash from virtual_path if present to avoid double slash
    const char* vpath = virtual_path;
    if (vpath[0] == '/') vpath++;
    
    snprintf(out_path, size, "%s/%s", state->physical_root_path, vpath);
}

static void normalize_path(char* path) {
    // Basic path normalization (handling .. and .)
    // This is a simple implementation for the shell simulation.
    // For a real shell, realpath() would be used on the physical path, 
    // but here we want to maintain the illusion of the virtual path.
    // ... Actually, resolving the physical path with realpath is safer 
    // to prevent breaking out of the sandbox.
}

static void cmd_ls(ShellState* state, const char* arg) {
    char target_vpath[MAX_VIRTUAL_PATH];
    char physical_path[MAX_PATH_LENGTH];

    // Determine target path
    if (arg == NULL || strlen(arg) == 0) {
        strncpy(target_vpath, state->current_virtual_path, MAX_VIRTUAL_PATH - 1);
    } else {
        // Handle absolute vs relative path
        if (arg[0] == '/') {
            strncpy(target_vpath, arg, MAX_VIRTUAL_PATH - 1);
        } else {
            snprintf(target_vpath, MAX_VIRTUAL_PATH, "%s/%s", state->current_virtual_path, arg);
        }
    }

    get_absolute_path(state, target_vpath, physical_path, MAX_PATH_LENGTH);

    DIR* dir = opendir(physical_path);
    if (dir) {
        struct dirent* entry;
        while ((dir && (entry = readdir(dir)) != NULL)) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            // Check if it's a directory
            char full_entry_path[MAX_PATH_LENGTH];
            snprintf(full_entry_path, MAX_PATH_LENGTH, "%s/%s", physical_path, entry->d_name);
            
            struct stat st;
            if (stat(full_entry_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    printf(ANSI_COLOR_BLUE "%s/  " ANSI_COLOR_RESET, entry->d_name);
                } else if (st.st_mode & S_IXUSR) {
                    printf(ANSI_COLOR_GREEN "%s*  " ANSI_COLOR_RESET, entry->d_name);
                } else {
                    printf("%s  ", entry->d_name);
                }
            } else {
                printf("%s  ", entry->d_name);
            }
        }
        printf("\n");
        closedir(dir);
    } else {
        printf(SHELL_ERROR_COLOR "ls: cannot access '%s': No such file or directory\n" ANSI_COLOR_RESET, arg ? arg : ".");
    }
}

static void cmd_cd(ShellState* state, const char* arg) {
    if (arg == NULL) {
        // cd with no args goes to home? default to /home
        strncpy(state->current_virtual_path, "/home", MAX_VIRTUAL_PATH - 1);
        return;
    }

    char new_vpath[MAX_VIRTUAL_PATH];
    if (arg[0] == '/') {
        strncpy(new_vpath, arg, MAX_VIRTUAL_PATH - 1);
    } else {
        // Handle ".."
        if (strcmp(arg, "..") == 0) {
            // Find last slash
            char* last_slash = strrchr(state->current_virtual_path, '/');
            if (last_slash != NULL && last_slash != state->current_virtual_path) {
                *last_slash = '\0';
                strncpy(new_vpath, state->current_virtual_path, MAX_VIRTUAL_PATH - 1);
                *last_slash = '/'; // Restore just in case
            } else {
                // Root
                strncpy(new_vpath, "/", MAX_VIRTUAL_PATH - 1);
            }
        } else if (strcmp(arg, ".") == 0) {
            return;
        } else {
            if (strcmp(state->current_virtual_path, "/") == 0) {
                 snprintf(new_vpath, MAX_VIRTUAL_PATH, "/%s", arg);
            } else {
                 snprintf(new_vpath, MAX_VIRTUAL_PATH, "%s/%s", state->current_virtual_path, arg);
            }
        }
    }

    // Verify existence and ensure it is inside root
    char physical_path[MAX_PATH_LENGTH];
    get_absolute_path(state, new_vpath, physical_path, MAX_PATH_LENGTH);
    
    // Resolve ".." in physical path to check for jailbreak attempt
    char resolved_path[MAX_PATH_LENGTH];
    if (realpath(physical_path, resolved_path) == NULL) {
         printf(SHELL_ERROR_COLOR "cd: %s: No such file or directory\n" ANSI_COLOR_RESET, arg);
         return;
    }

    // Security check: ensure resolved path starts with physical_root_path
    // Note: realpath might resolve symlinks, so we must compare against resolved root
    char resolved_root[MAX_PATH_LENGTH];
    if (realpath(state->physical_root_path, resolved_root) == NULL) {
        printf(SHELL_ERROR_COLOR "Error resolving root path\n" ANSI_COLOR_RESET);
        return;
    }

    if (strncmp(resolved_path, resolved_root, strlen(resolved_root)) != 0) {
        printf(SHELL_ERROR_COLOR "cd: Access denied (Outside of world)\n" ANSI_COLOR_RESET);
        return;
    }

    struct stat st;
    if (stat(resolved_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        // Update virtual path (simplified, doesn't perfectly handle complex relative paths like a/b/../c)
        // ideally we should reconstruct virtual path from resolved path relative to root
        char* relative_part = resolved_path + strlen(resolved_root);
        if (strlen(relative_part) == 0) {
            strncpy(state->current_virtual_path, "/", MAX_VIRTUAL_PATH - 1);
        } else {
            strncpy(state->current_virtual_path, relative_part, MAX_VIRTUAL_PATH - 1);
        }
    } else {
        printf(SHELL_ERROR_COLOR "cd: %s: Not a directory\n" ANSI_COLOR_RESET, arg);
    }
}

static void cmd_cat(ShellState* state, const char* arg) {
    if (arg == NULL) {
        printf("cat: missing operand\n");
        return;
    }

    char target_vpath[MAX_VIRTUAL_PATH];
    if (arg[0] == '/') {
        strncpy(target_vpath, arg, MAX_VIRTUAL_PATH - 1);
    } else {
        if (strcmp(state->current_virtual_path, "/") == 0) {
             snprintf(target_vpath, MAX_VIRTUAL_PATH, "/%s", arg);
        } else {
             snprintf(target_vpath, MAX_VIRTUAL_PATH, "%s/%s", state->current_virtual_path, arg);
        }
    }

    char physical_path[MAX_PATH_LENGTH];
    get_absolute_path(state, target_vpath, physical_path, MAX_PATH_LENGTH);

    FILE* f = fopen(physical_path, "r");
    if (f) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), f)) {
            printf("%s", buffer);
        }
        printf("\n");
        fclose(f);
    } else {
        printf(SHELL_ERROR_COLOR "cat: %s: No such file or directory\n" ANSI_COLOR_RESET, arg);
    }
}

void enter_navi_shell(GameState* game_state) {
    ShellState state;
    // Set root to <project_root>/world
    snprintf(state.physical_root_path, MAX_PATH_LENGTH, "%s/%s", game_state->paths.base_path, VIRTUAL_ROOT);
    
    // Ensure world directory exists
    struct stat st;
    if (stat(state.physical_root_path, &st) == -1) {
        printf(SHELL_ERROR_COLOR "Error: World directory not found at %s\n" ANSI_COLOR_RESET, state.physical_root_path);
        return;
    }

    // Default start path
    strncpy(state.current_virtual_path, "/home", MAX_VIRTUAL_PATH - 1);

    clear_screen();
    printf("NAVI Shell v1.0\n");
    printf("Type 'help' for commands.\n\n");

    char* line;
    char prompt[MAX_VIRTUAL_PATH + 32];

    while (1) {
        snprintf(prompt, sizeof(prompt), "%snv@wired%s:%s%s$ ", 
                 SHELL_PROMPT_COLOR, ANSI_COLOR_RESET, 
                 SHELL_PATH_COLOR, state.current_virtual_path);
        
        line = linenoise(prompt);
        if (line == NULL) break; // EOF

        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
            
            // Tokenize
            char* cmd = strtok(line, " ");
            char* arg = strtok(NULL, " ");

            if (cmd) {
                if (strcmp(cmd, "exit") == 0) {
                    free(line);
                    break;
                } else if (strcmp(cmd, "ls") == 0) {
                    cmd_ls(&state, arg);
                } else if (strcmp(cmd, "cd") == 0) {
                    cmd_cd(&state, arg);
                } else if (strcmp(cmd, "pwd") == 0) {
                    printf("%s\n", state.current_virtual_path);
                } else if (strcmp(cmd, "cat") == 0) {
                    cmd_cat(&state, arg);
                } else if (strcmp(cmd, "whoami") == 0) {
                    printf("lain\n");
                } else if (strcmp(cmd, "clear") == 0) {
                    clear_screen();
                } else if (strcmp(cmd, "help") == 0) {
                    printf("Available commands: ls, cd, pwd, cat, whoami, clear, exit\n");
                } else {
                    printf("%s: command not found\n", cmd);
                }
            }
        }
        free(line);
    }
}
