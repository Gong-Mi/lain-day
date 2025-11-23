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
#include "scenes.h"       // For init_scene_examine_navi

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

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }

    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    // --- Initialize game_state directly for demo ---
    // Hardcode PlayerState
    strncpy(game_state.player_state.location, "lain_room", MAX_NAME_LENGTH - 1);
    game_state.player_state.credit_level = 1;
    game_state.player_state.inventory_count = 0;
    strncpy(game_state.player_state.unlocked_commands[0], "help", MAX_NAME_LENGTH - 1);
    game_state.player_state.unlocked_commands_count = 1;

    // Hardcode mock Action entries for NAVI choices
    // Action: navi_shutdown
    strncpy(game_state.all_actions[0].id, "navi_shutdown", MAX_NAME_LENGTH - 1);
    strncpy(game_state.all_actions[0].type_str, "story_change", MAX_NAME_LENGTH - 1); // Mock type
    game_state.all_actions[0].payload_json = NULL; // No actual payload for mock
    game_state.action_count = 1;

    // Action: navi_reboot
    strncpy(game_state.all_actions[1].id, "navi_reboot", MAX_NAME_LENGTH - 1);
    strncpy(game_state.all_actions[1].type_str, "story_change", MAX_NAME_LENGTH - 1); // Mock type
    game_state.all_actions[1].payload_json = NULL; // No actual payload for mock
    game_state.action_count++;

    // Action: navi_connect
    strncpy(game_state.all_actions[2].id, "navi_connect", MAX_NAME_LENGTH - 1);
    strncpy(game_state.all_actions[2].type_str, "story_change", MAX_NAME_LENGTH - 1); // Mock type
    game_state.all_actions[2].payload_json = NULL; // No actual payload for mock
    game_state.action_count++;

    printf("PlayerState and Actions hardcoded for demo.\n");

    // Comment out file-based data loading as it's bypassed
    /*
    if (!load_player_state("./character.json", &game_state)) {
        fprintf(stderr, "Failed to load player data. Exiting.\n");
        return 1;
    }
    
    if (!load_map_data("./map", &game_state)) {
        fprintf(stderr, "Failed to load map data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }

    if (!load_items_data("./items.json", &game_state)) {
        fprintf(stderr, "Failed to load items data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }

    if (!load_actions_data("./actions.json", &game_state)) {
        fprintf(stderr, "Failed to load actions data. Exiting.\n");
        cleanup_game_state(&game_state);
        return 1;
    }
    printf("All data loaded successfully.\n");
    */

    // --- Game Loop ---
    int running = 1;
    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;

    // Initialize the Navi scene directly for demonstration
    init_scene_examine_navi(&current_scene);
    strncpy(game_state.player_state.location, current_scene.location_id, MAX_NAME_LENGTH - 1); // Set initial location
    
    // Initial render of the hardcoded scene
    render_current_scene(&current_scene);


    while (running) {
        get_player_input(input_buffer, sizeof(input_buffer));
        // scene_changed = 0; // Reset for next loop

        if (strcmp(input_buffer, "quit") == 0) {
            running = 0;
        } else if (is_numeric(input_buffer)) {
            int choice_index = atoi(input_buffer) - 1;
            if (choice_index >= 0 && choice_index < current_scene.choice_count) {
                const char* action_id = current_scene.choices[choice_index].action_id;
                // For this demo, execute_action will simply print the action ID
                printf("Executing action: %s\n", action_id);
                // In a real game, execute_action would update game_state and potentially change scene_changed
                // For now, we'll keep the scene static after choice, unless action changes it
                
                // For demo, if action changes scene, we would call a new init_scene_X
                // For now, let's just re-render to simulate a response if we don't change scene
                render_current_scene(&current_scene); 

            } else {
                printf("Invalid choice number.\n");
            }
        } else {
            // Placeholder for command execution, will always print invalid for now
            printf("Invalid command or input for this scene. Please choose a number.\n");
        }
    }

    // --- Save and Cleanup ---
    printf("\nSaving game state...\n");
    // For this hardcoded demo, we don't save game state or cleanup dynamically allocated cJSON objects.
    // The previous debug code to print CWD should also be removed.
    // cleanup_game_state(&game_state);

    printf("\nLain-day C version exiting.\n");
    return 0;
}