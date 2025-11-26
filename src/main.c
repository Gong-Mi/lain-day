#include <stdio.h>
#include <stdlib.h> // For atoi
#include <string.h> // For memset and strcmp
#include <ctype.h>  // For isdigit
#include <unistd.h> // For getcwd

#include "game_types.h"
#include "data_loader.h"
#include "map_loader.h"
#include "story_parser.h" // Still needed for compilation, but not for the hardcoded scene logic here
#include "executor.h"
#include "string_table.h" // For get_string_by_id
#include "scenes.h"       // For transition_to_scene

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
        printf("%s\n", get_string_by_id(scene->text_content_ids[i]));
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

    // --- GameState Initialization ---
    // The master GameState struct is created and zeroed out.
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    // --- Load all game data ---
    // The engine now loads all data from the JSON and map files at startup.
    if (!load_player_state("./character.json", &game_state)) {
        fprintf(stderr, "Failed to load player data. Exiting.\n");
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
    // The final game state is saved back to character.json before exiting.
    printf("\nSaving game state...\n");
    if (!save_game_state("./character.json", &game_state)) {
        fprintf(stderr, "Error: Failed to save game state.\n");
    } else {
        printf("Game state saved successfully.\n");
    }
    
    cleanup_game_state(&game_state);

    printf("\nLain-day C version exiting.\n");
    return 0;
}
