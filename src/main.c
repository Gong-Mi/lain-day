#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <libgen.h> // For dirname
#include <errno.h> // Required for errno and strerror

#include "build_info.h"
#include "game_types.h"
#include "data_loader.h"
#include "map_loader.h"
#include "story_parser.h"
#include "executor.h"
#include "string_table.h"
#include "string_table.h"
#include "scenes.h"
#include "../include/ansi_colors.h"
#include "../include/project_status.h"
#include "flag_system.h"
#include "compression_util.h" // Include for zlib functions
#include <zlib.h> // Required for Z_OK, Z_DEFAULT_COMPRESSION, etc.

// --- Path Management Struct ---
typedef struct {
    char base_path[MAX_PATH_LENGTH];
    char default_character_file[MAX_PATH_LENGTH];
    char items_file[MAX_PATH_LENGTH];
    char actions_file[MAX_PATH_LENGTH];
    char map_dir[MAX_PATH_LENGTH];
    char session_root_dir[MAX_PATH_LENGTH];
} GamePaths;

// --- Function Prototypes ---

// System Helpers
void clear_screen();
void print_game_time(int time_of_day);

// Path Management
void get_base_path(char* exe_path, char* base_path, size_t size);
void init_paths(char* argv0, GamePaths* paths);

// Game Loop Components
int copy_file(const char *src_path, const char *dest_path);
void print_colored_line(const char* line, const GameState* game_state);
void render_current_scene(const StoryScene* scene, const GameState* game_state);
void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index);
int is_numeric(const char* str);


int main(int argc, char *argv[]) {
    printf("Lain-day C version starting...\n");
    printf("Build Info - OS: %s, Arch: %s\n", BUILD_OS, BUILD_ARCH);

    // --- Zlib Compression Test ---
    const char* original_string = "This is a test string for zlib compression and decompression functionality.";
    unsigned char* compressed_data = NULL;
    unsigned long compressed_data_len = 0;
    unsigned char* decompressed_data = NULL;
    unsigned long decompressed_data_len = 0;

    printf("Original string: \"%s\"\n", original_string);

    if (compress_string(original_string, &compressed_data, &compressed_data_len) == Z_OK) {
        printf("Compressed data length: %lu\n", compressed_data_len);
        // For demonstration, print first few bytes of compressed data
        printf("Compressed data (first 10 bytes): ");
        for (int i = 0; i < 10 && i < compressed_data_len; i++) {
            printf("%02x ", compressed_data[i]);
        }
        printf("...\n");

        if (decompress_string(compressed_data, compressed_data_len, &decompressed_data, &decompressed_data_len) == Z_OK) {
            printf("Decompressed data: \"%s\"\n", decompressed_data);
            if (strcmp(original_string, (char*)decompressed_data) == 0) {
                printf("Compression/Decompression successful and data matches!\n");
            } else {
                printf("Error: Decompressed data does not match original!\n");
            }
        } else {
            fprintf(stderr, "Error: Zlib decompression failed.\n");
        }
    } else {
        fprintf(stderr, "Error: Zlib compression failed.\n");
    }

    // Free allocated memory
    if (compressed_data) free(compressed_data);
    if (decompressed_data) free(decompressed_data);
    // --- End Zlib Compression Test ---
    
    GamePaths paths;
    init_paths(argv[0], &paths);

    char session_name[MAX_NAME_LENGTH];
    char session_dir_path[MAX_PATH_LENGTH];
    char character_session_file_path[MAX_PATH_LENGTH];
    
    int arg_index = 1;
    bool is_test_mode = false;

    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-d") == 0) {
            is_test_mode = true;
            printf("Running in test mode (-d). Temporary session.\n");
        } else {
            fprintf(stderr, "Warning: Unknown argument '%s'. Ignoring.\n", argv[arg_index]);
        }
        arg_index++;
    }

    // --- Session Management ---
    if (is_test_mode) {
        printf("Test mode enabled. Using default character state without session persistence.\n");
        strncpy(character_session_file_path, paths.default_character_file, MAX_PATH_LENGTH - 1);
        // ... rest of test mode logic
        } else { // Normal mode - persistent session
            if (argc > arg_index) {
                strncpy(session_name, argv[arg_index], MAX_NAME_LENGTH - 1);
                session_name[MAX_NAME_LENGTH - 1] = '\0';
                printf("Using session name from argument: %s\n", session_name);
                arg_index++;
            } else {
                bool name_valid = false;
                while(!name_valid) {
                    printf("请输入会话名称 (例如 'new_game' 或 'resume_game'): ");
                    if (fgets(session_name, MAX_NAME_LENGTH, stdin) != NULL) {
                        session_name[strcspn(session_name, "\n")] = 0; // Remove newline
                        if (strlen(session_name) > 0) {
                            name_valid = true;
                        } else {
                            printf("会话名称不能为空，请重新输入。\n");
                        }
                    } else {
                        fprintf(stderr, "Error: Failed to read session name.\n");
                        return 1;
                    }
                }
            }

        // Create session directories
        mkdir(paths.session_root_dir, 0755);
        snprintf(session_dir_path, MAX_PATH_LENGTH, "%s/%s", paths.session_root_dir, session_name);
        mkdir(session_dir_path, 0755);
        
        snprintf(character_session_file_path, MAX_PATH_LENGTH, "%s/character.json", session_dir_path);

        if (access(character_session_file_path, F_OK) == -1) {
            printf("Creating new session '%s'.\n", session_name);
            if (!copy_file(paths.default_character_file, character_session_file_path)) {
                fprintf(stderr, "Error: Failed to copy default character.json to session.\n");
                return 1;
            }
        } else {
            printf("Resuming session '%s'.\n", session_name);
        }
    }
        
    // --- Load all game data using paths struct ---
    if (!load_player_state(character_session_file_path, &game_state)) {
        // ... error handling
        return 1;
    }
    printf("Player data loaded.\n");
#ifdef USE_DEBUG_LOGGING
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Main: Player location after loading: '%s'\n", game_state.player_state.location);
#endif
#endif

    if (!load_map_data(paths.map_dir, &game_state)) {
        // ... error handling
        return 1;
    }
    printf("Map data loaded.\n");

    if (!load_items_data(paths.items_file, &game_state)) {
        // ... error handling
        return 1;
    }
    printf("Items data loaded.\n");
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: main: g_string_table[TEXT_DOWNSTAIRS_DESC1] = %s\n", g_string_table[TEXT_DOWNSTAIRS_DESC1]);
    fprintf(stderr, "DEBUG: main: g_string_table[TEXT_LAIN_ROOM_TITLE] = %s\n", g_string_table[TEXT_LAIN_ROOM_TITLE]);
    fprintf(stderr, "DEBUG: main: get_string_by_id(70) returns: %s\n", get_string_by_id(70));
    fprintf(stderr, "DEBUG: main: get_string_by_id(TEXT_DOWNSTAIRS_DESC1) returns: %s\n", get_string_by_id(TEXT_DOWNSTAIRS_DESC1));
    fprintf(stderr, "DEBUG: main: get_string_by_id(25) returns: %s\n", get_string_by_id(25));
    fprintf(stderr, "DEBUG: main: get_string_by_id(TEXT_LAIN_ROOM_TITLE) returns: %s\n", get_string_by_id(TEXT_LAIN_ROOM_TITLE));
#endif

    // --- Game Loop ---
    // ... [The rest of the main function remains largely the same] ...
    int running = 1;
    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;

    if (!transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
        fprintf(stderr, "Failed to initialize starting scene: %s\n", game_state.current_story_file);
        return 1;
    }
    
    render_current_scene(&current_scene, &game_state);

    while (running) {
        get_next_input(input_buffer, sizeof(input_buffer), argc, argv, &arg_index);

        if (strcmp(input_buffer, "quit") == 0 || strcmp(input_buffer, "0") == 0) {
            running = 0;
        } else if (is_numeric(input_buffer)) {
            // ... [choice processing logic] ...
            int target_visible_index = atoi(input_buffer);
            int visible_choice_count = 0;
            int target_array_index = -1;

            for (int i = 0; i < current_scene.choice_count; i++) {
                const StoryChoice* choice = &current_scene.choices[i];
                int is_selectable = 0;
                if (choice->condition.flag_name[0] == '\0') {
                    is_selectable = 1;
                } else {
                    const char* flag_value_str = hash_table_get(game_state.flags, choice->condition.flag_name);
                    if (flag_value_str != NULL) {
                        if (atoi(flag_value_str) == choice->condition.required_value) {
                            is_selectable = 1;
                        }
                    }
                }
                
                if (is_selectable) {
                    visible_choice_count++;
                    if (visible_choice_count == target_visible_index) {
                        target_array_index = i;
                        break;
                    }
                }
            }

            if (target_array_index != -1) {
                const char* action_id = current_scene.choices[target_array_index].action_id;
                
                if (execute_action(action_id, &game_state)) { 
                    if (!transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
                        fprintf(stderr, "Failed to transition to scene %s.\n", game_state.current_story_file);
                    }
                }
                render_current_scene(&current_scene, &game_state);

            } else {
                printf("Invalid choice number.\n");
            }

        } else {
            execute_command(input_buffer, &game_state);
            render_current_scene(&current_scene, &game_state); 
        }
    }

    // --- Save and Cleanup ---
    if (!is_test_mode) {
        printf("\nSaving game state...\n");
        if (!save_game_state(character_session_file_path, &game_state)) {
            fprintf(stderr, "Error: Failed to save game state to %s.\n", character_session_file_path);
        } else {
            printf("Game state saved successfully.\n");
        }
    } else {
        printf("\nTest mode: Skipping game state saving.\n");
    }
    
    cleanup_game_state(&game_state);

    printf("\nLain-day C version exiting.\n");
    return 0;
}


// --- Function Definitions ---

void get_base_path(char* exe_path, char* base_path, size_t size) {
    char* exe_dir = dirname(exe_path);
    snprintf(base_path, size, "%s/..", exe_dir);
}

void init_paths(char* argv0, GamePaths* paths) {
    char exe_path[MAX_PATH_LENGTH];
    char* resolved_path = realpath(argv0, exe_path);
    if (resolved_path == NULL) {
        fprintf(stderr, "Error: Could not resolve executable path for '%s'. %s\n", argv0, strerror(errno));
        exit(EXIT_FAILURE);
    }
    char install_root[MAX_PATH_LENGTH];
    get_base_path(exe_path, install_root, sizeof(install_root));
    snprintf(paths->base_path, sizeof(paths->base_path), "%s", install_root);
    snprintf(paths->default_character_file, sizeof(paths->default_character_file), "%s/character.json", paths->base_path);
    snprintf(paths->items_file, sizeof(paths->items_file), "%s/items.json", paths->base_path);
    snprintf(paths->map_dir, sizeof(paths->map_dir), "%s/map", paths->base_path);
    snprintf(paths->session_root_dir, sizeof(paths->session_root_dir), "%s/session", paths->base_path);
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

void print_colored_line(const char* line, const GameState* game_state) {
    if (line == NULL) {
        printf("\n");
        fflush(stdout);
        return;
    }

    // List of speakers and their colors.
    struct {
        const char* name_prefix;
        const char* color_code;
    } speakers[] = {
        {"Alice:", ALICE_COLOR},
        {"Chisa:", CHISA_COLOR},
        {"Father:", FATHER_COLOR},
        {"米良柊子:", FUYUKO_MIRA_COLOR}, // Doctor
        {"Mira:", LAINS_SISTER_MIRA_COLOR}, // Lain's Sister
        {"幽灵:", ANSI_COLOR_RED}, // Ghost
        {"爸爸:", FATHER_COLOR}, // Father
        {"妈妈:", ANSI_COLOR_MAGENTA}, // Mom
    };
    int num_speakers = sizeof(speakers) / sizeof(speakers[0]);

#ifdef USE_TYPEWRITER_EFFECT
    // --- Typewriter Effect Implementation ---
    const char* content_start = line;
    for (int i = 0; i < num_speakers; i++) {
        size_t prefix_len = strlen(speakers[i].name_prefix);
        if (strncmp(line, speakers[i].name_prefix, prefix_len) == 0) {
            // Found a speaker prefix, print it at once
            printf("%s%s%s", speakers[i].color_code, speakers[i].name_prefix, ANSI_COLOR_RESET);
            content_start += prefix_len;
            break;
        }
    }

    // Print the rest of the line char by char
    for (int i = 0; content_start[i] != '\0'; i++) {
        putchar(content_start[i]);
        fflush(stdout);
        usleep((useconds_t)(game_state->typewriter_delay * 1000000));
    }

    printf("\n");
    fflush(stdout);

#else
    // --- Standard Instant Implementation ---
    for (int i = 0; i < num_speakers; i++) {
        size_t prefix_len = strlen(speakers[i].name_prefix);
        if (strncmp(line, speakers[i].name_prefix, prefix_len) == 0) {
            // Found a speaker prefix
            printf("%s%s%s%s\n", speakers[i].color_code, speakers[i].name_prefix, ANSI_COLOR_RESET, line + prefix_len);
            return;
        }
    }

    // No speaker detected, print the line as is
    printf("%s\n", line);
#endif
}

void render_current_scene(const StoryScene* scene, const GameState* game_state) {
    // clear_screen(); // Clear screen for each scene render
    print_game_time(game_state->time_of_day); // Print time at the top-left
    #ifdef USE_DEBUG_LOGGING
#ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Entering render_current_scene.\n");
    fprintf(stderr, "DEBUG: render_current_scene: scene ptr: %p\n", (void*)scene);
    fprintf(stderr, "DEBUG: Rendering scene: %s (ID: %s)\n", scene->name, scene->scene_id);
#endif
#endif
    if (scene == NULL) {
        printf("Error: Scene is NULL.\n");
        return;
    }

    printf("\n========================================\n");
    if (scene->location_id[0] != '\0') {
        // This part doesn't need typewriter effect
        printf("Location: %s\n", scene->location_id);
    }
    printf("========================================\n");

    for (int i = 0; i < scene->text_line_count; i++) {
#ifdef USE_STRING_DEBUG_LOGGING
        fprintf(stderr, "DEBUG:   Printing StringID: %d (%s)\n", scene->text_content_ids[i], get_string_by_id(scene->text_content_ids[i]));
#endif
        print_colored_line(get_string_by_id(scene->text_content_ids[i]), game_state);
    }

    if (scene->choice_count > 0) {
        printf("\n--- Choices ---\n");
        int visible_choice_index = 1;
        for (int i = 0; i < scene->choice_count; i++) {
            const StoryChoice* choice = &scene->choices[i];
            int is_selectable = 0;

            // Check if there is a condition
            if (choice->condition.flag_name[0] == '\0') {
                is_selectable = 1; // No condition, always selectable
            } else {
                const char* flag_value_str = hash_table_get(game_state->flags, choice->condition.flag_name);
                if (flag_value_str != NULL) {
                    // Flag exists, compare its value
                    int current_value = atoi(flag_value_str);
                    if (current_value == choice->condition.required_value) {
                        is_selectable = 1;
                    }
                }
                // If flag is not set, is_selectable remains 0.
            }

            if (is_selectable) {
                printf("%d. %s\n", visible_choice_index++, get_string_by_id(choice->text_id));
            } else {
                // Print disabled choice in gray and without a number
                printf("   %s%s%s\n", ANSI_COLOR_BRIGHT_BLACK, get_string_by_id(choice->text_id), ANSI_COLOR_RESET);
            }
        }
        printf("---------------\n");
    }
}

void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index) {
    if (argc > 1 && *arg_index < argc) {
        // Automated mode: get input from command-line arguments
        strncpy(buffer, argv[*arg_index], buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        printf("> %s\n", buffer); // Echo the automated input
        (*arg_index)++;
    } else {
        // Manual mode: get input from stdin
        printf("> ");
        if (fgets(buffer, buffer_size, stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
        }
    }
}

int is_numeric(const char* str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) {
            return 0;
        }
    }
    return 1;
}

void print_game_time(int time_of_day) {
    int hours = time_of_day / 60;
    int minutes = time_of_day % 60;
    printf(ANSI_COLOR_YELLOW "[%02d:%02d]" ANSI_COLOR_RESET "\n", hours, minutes);
}

void clear_screen() {
    printf("\033[2J\033[H");
}