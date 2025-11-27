#include <stdio.h>
#include <stdlib.h> // For atoi
#include <string.h> // For memset and strcmp, strlen, strncpy
#include <ctype.h>  // For isdigit
#include <unistd.h> // For getcwd, access
#include <sys/stat.h> // For mkdir
#include <stdbool.h> // For bool type

#include "build_info.h"   // For BUILD_OS, BUILD_ARCH
#include "game_types.h"
#include "data_loader.h"
#include "map_loader.h"
#include "story_parser.h" // Still needed for compilation, but not for the hardcoded scene logic here
#include "executor.h"
#include "string_table.h" // For get_string_by_id
#include "scenes.h"       // For transition_to_scene
#include "../include/ansi_colors.h" // For ANSI color codes
#include "../include/project_status.h" // Development status document
#include "flag_system.h" // Include for hash table flag system

// Helper function to copy a file
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

// Helper function to print a line with typewriter effect and colored speaker name
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

// --- Helper Functions for Game Loop ---
void render_current_scene(const StoryScene* scene, const GameState* game_state) {
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

// Reads player input into a buffer, from argv or stdin
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

// Checks if a string contains only digits
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

int main(int argc, char *argv[]) {
    printf("Lain-day C version starting...\n");
    printf("Build Info - OS: %s, Arch: %s\n", BUILD_OS, BUILD_ARCH);

    const char* SESSION_DIR_NAME = "session";
    const char* DEFAULT_CHARACTER_FILE = "./character.json"; // Path to the pristine default character file
    char session_name[MAX_NAME_LENGTH];
    char session_dir_path[MAX_PATH_LENGTH];
    char character_session_file_path[MAX_PATH_LENGTH];
    
    int arg_index = 1; // Start reading args from index 1 (0 is program name)
    bool is_test_mode = false; // Flag for test mode

    // The master GameState struct is created and zeroed out.
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    // Parse command-line flags before session name
    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-d") == 0) {
            is_test_mode = true;
            printf("Running in test mode (-d). Temporary session.\n");
        } else {
            fprintf(stderr, "Warning: Unknown argument '%s'. Ignoring.\n", argv[arg_index]);
        }
        arg_index++;
    }

    // Get current working directory for relative paths
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error: Failed to get current working directory.\n");
        return 1;
    }
        // --- Session Management ---
        if (is_test_mode) {
            printf("Test mode enabled. Using default character state without session persistence.\n");
            strncpy(character_session_file_path, DEFAULT_CHARACTER_FILE, MAX_PATH_LENGTH - 1);
            character_session_file_path[MAX_PATH_LENGTH - 1] = '\0';
            if (argc > arg_index) { // Set session_name from argument for logging in test mode
                strncpy(session_name, argv[arg_index], MAX_NAME_LENGTH - 1);
                session_name[MAX_NAME_LENGTH - 1] = '\0';
                printf("Test session name: %s\n", session_name);
                arg_index++;
            } else {
                strncpy(session_name, "test_session_default", MAX_NAME_LENGTH - 1);
                session_name[MAX_NAME_LENGTH - 1] = '\0';
                printf("No test session name provided. Using default: %s\n", session_name);
            }
        } else { // Normal mode - persistent session
            if (argc > arg_index) { // Get session name from argument or prompt
                strncpy(session_name, argv[arg_index], MAX_NAME_LENGTH - 1);
                session_name[MAX_NAME_LENGTH - 1] = '\0';
                printf("Using session name from argument: %s\n", session_name);
                arg_index++;
            } else {
                printf("请输入会话名称 (例如 'new_game' 或 'resume_game'): ");
                if (fgets(session_name, MAX_NAME_LENGTH, stdin) != NULL) {
                    session_name[strcspn(session_name, "\n")] = 0;
                } else {
                    fprintf(stderr, "Error: Failed to read session name.\n");
                    return 1;
                }
            }
    
            snprintf(session_dir_path, MAX_PATH_LENGTH, "%s/session/%s", cwd, session_name);
            #ifdef _WIN32
                _mkdir(session_dir_path);
            #else
                mkdir(session_dir_path, 0755);
            #endif
    
            snprintf(character_session_file_path, MAX_PATH_LENGTH, "%s/character.json", session_dir_path);
    
            if (access(character_session_file_path, F_OK) == -1) {
                printf("Creating new session '%s'. Copying default character state...\n", session_name);
                if (!copy_file(DEFAULT_CHARACTER_FILE, character_session_file_path)) {
                    fprintf(stderr, "Error: Failed to copy default character.json to session.\n");
                    return 1;
                }
            } else {
                printf("Resuming session '%s'.\n", session_name);
            }
        }
        
        // --- Load all game data ---
    if (!load_player_state(character_session_file_path, &game_state)) {
        fprintf(stderr, "Failed to load player data from %s. Exiting.\n", character_session_file_path);
        return 1;
    }
    printf("Player data loaded successfully.\n");

    if (!load_map_data("./map", &game_state)) {
        fprintf(stderr, "Failed to load map data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }
    printf("Map data loaded successfully.\n");

    if (!load_items_data("./items.json", &game_state)) {
        fprintf(stderr, "Failed to load items data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }
    printf("Items data loaded successfully.\n");

    if (!load_actions_data("./actions.json", &game_state)) {
        fprintf(stderr, "Failed to load actions data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }
    printf("Actions data loaded successfully.\n");


    // --- Game Loop ---
    int running = 1;
    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;

    // Initialize the first scene based on the state loaded from character.json
    if (!transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
        fprintf(stderr, "Failed to initialize starting scene from story file: %s\n", game_state.current_story_file);
        return 1;
    }
    
    // Initial render of the scene
    render_current_scene(&current_scene, &game_state);


    // The main loop handles player input, routing it to either the choice handler
    // or the text command executor.
    while (running) {
        get_next_input(input_buffer, sizeof(input_buffer), argc, argv, &arg_index);

        if (strcmp(input_buffer, "quit") == 0 || strcmp(input_buffer, "0") == 0) {
            running = 0;
        } else if (is_numeric(input_buffer)) {
            int target_visible_index = atoi(input_buffer);
            int visible_choice_count = 0;
            int target_array_index = -1;

            // Find the actual array index corresponding to the visible choice number
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
            // For non-numeric input, execute as a text command.
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
