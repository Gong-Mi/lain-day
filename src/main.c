#include <stdio.h>
#include <stdlib.h> // For atoi
#include <string.h> // For memset and strcmp, strlen, strncpy
#include <ctype.h>  // For isdigit
#include <unistd.h> // For getcwd, access
#include <sys/stat.h> // For mkdir

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

// Helper function to print a line with colored speaker name if detected
void print_colored_line(const char* line) {
    if (line == NULL) {
        printf("\n");
        return;
    }

    // List of speakers and their colors.
    // Order matters if prefixes overlap (e.g., "Mira" and "Mira Sister").
    // We assume "米良柊子" and "Mira" (Lain's Sister) are distinct prefixes.
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
        // Add other speakers here as needed
    };
    int num_speakers = sizeof(speakers) / sizeof(speakers[0]);

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
}

// --- Helper Functions for Game Loop ---
void render_current_scene(const StoryScene* scene) {
    if (scene == NULL) {
        printf("Error: Scene is NULL.\n");
        return;
    }

    printf("\n========================================\n");
    if (scene->location_id[0] != '\0') {
        printf("Location: %s\n", scene->location_id);
    }
    printf("========================================\n");

    for (int i = 0; i < scene->text_line_count; i++) {
        print_colored_line(get_string_by_id(scene->text_content_ids[i]));
    }

    if (scene->choice_count > 0) {
        printf("\n--- Choices ---\n");
        for (int i = 0; i < scene->choice_count; i++) {
            printf("%d. %s\n", i + 1, get_string_by_id(scene->choices[i].text_id));
        }
        printf("---------------\n");
    }
}

// Reads player input into a buffer
void get_player_input(char* buffer, int buffer_size) {
    printf("> ");
    if (fgets(buffer, buffer_size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
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

int main() {
    printf("Lain-day C version starting...\n");
    printf("Build Info - OS: %s, Arch: %s\n", BUILD_OS, BUILD_ARCH);

    const char* SESSION_DIR_NAME = "session";
    const char* DEFAULT_CHARACTER_FILE = "./character.json"; // Path to the pristine default character file
    char session_name[MAX_NAME_LENGTH];
    char session_dir_path[MAX_PATH_LENGTH];
    char character_session_file_path[MAX_PATH_LENGTH];

    // Get current working directory for relative paths
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error: Failed to get current working directory.\n");
        return 1;
    }
    // --- Session Management ---
    printf("请输入会话名称 (例如 'new_game' 或 'resume_game'): ");
    if (fgets(session_name, MAX_NAME_LENGTH, stdin) != NULL) {
        session_name[strcspn(session_name, "\n")] = 0; // Remove trailing newline
    } else {
        fprintf(stderr, "Error: Failed to read session name.\n");
        return 1;
    }

    // Construct path for session directory
    snprintf(session_dir_path, MAX_PATH_LENGTH, "%s/session/%s", cwd, session_name);

    // Create session directory if it doesn't exist
    #ifdef _WIN32
        _mkdir(session_dir_path); // Windows
    #else
        mkdir(session_dir_path, 0755); // Linux/Unix
    #endif

    // Construct path for character.json within the session directory
    snprintf(character_session_file_path, MAX_PATH_LENGTH, "%s/character.json", session_dir_path);

    // If session-specific character.json doesn't exist, copy from default
    if (access(character_session_file_path, F_OK) == -1) {
        printf("Creating new session '%s'. Copying default character state...\n", session_name);
        if (!copy_file(DEFAULT_CHARACTER_FILE, character_session_file_path)) {
            fprintf(stderr, "Error: Failed to copy default character.json to session.\n");
            return 1;
        }
    } else {
        printf("Resuming session '%s'.\n", session_name);
    }
    
    // The master GameState struct is created and zeroed out.
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    // --- Load all game data ---
    // The engine now loads all data from the JSON and map files at startup.
    // character.json serves as the initial default character state for a new game,
    // and also the save file for current game progress.
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
    render_current_scene(&current_scene);


    // The main loop handles player input, routing it to either the choice handler
    // or the text command executor.
    while (running) {
        get_player_input(input_buffer, sizeof(input_buffer));

        if (strcmp(input_buffer, "quit") == 0) {
            running = 0;
        } else if (is_numeric(input_buffer)) {
            int choice_index = atoi(input_buffer) - 1;
            if (choice_index >= 0 && choice_index < current_scene.choice_count) {
                const char* action_id = current_scene.choices[choice_index].action_id;
                
                // execute_action handles the game logic for the choice.
                // If it returns true, the story file has changed.
                if (execute_action(action_id, &game_state)) { 
                    // transition_to_scene loads the new C-based scene.
                    if (!transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
                        fprintf(stderr, "Failed to transition to scene %s.\n", game_state.current_story_file);
                    }
                }
                render_current_scene(&current_scene);

            } else {
                printf("Invalid choice number.\n");
            }
        } else {
            // For non-numeric input, execute as a text command.
            execute_command(input_buffer, &game_state);
            render_current_scene(&current_scene); 
        }
    }

    // --- Save and Cleanup ---
    // The final game state is saved back to the session-specific character.json before exiting.
    printf("\nSaving game state...\n");
    if (!save_game_state(character_session_file_path, &game_state)) {
        fprintf(stderr, "Error: Failed to save game state to %s.\n", character_session_file_path);
    } else {
        printf("Game state saved successfully.\n");
    }
    
    cleanup_game_state(&game_state);

    printf("\nLain-day C version exiting.\n");
    return 0;
}
