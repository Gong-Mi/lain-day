#include <stdio.h>
#include <string.h>
#include "game_types.h"
#include "scenes.h"
#include "data_loader.h"
#include "string_table.h"

// --- Forward Declarations ---
static void print_usage(const char* prog_name);
static void debug_print_scene(const StoryScene* scene, const GameState* game_state);

// --- Main Function ---
int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* scene_id_to_debug = argv[1];
    printf("--- Debugging Scene: %s ---\n\n", scene_id_to_debug);

    // 1. Initialize a minimal GameState
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));

    // 2. Game data is linked at compile time, no loading needed.
    // We just need a valid GameState object to pass to functions.
    // NOTE: A real game would load string tables here if they weren't hardcoded.

    // 3. Initialize a StoryScene struct and transition to the target scene
    StoryScene scene;
    if (!transition_to_scene(scene_id_to_debug, &scene, &game_state)) {
        fprintf(stderr, "ERROR: Failed to transition to scene '%s'. Is it registered in scenes.c?\n", scene_id_to_debug);
        // cleanup_game_state is not strictly necessary here as GameState is simple, but good practice
        return 1;
    }

    // 4. Print the parsed scene data
    debug_print_scene(&scene, &game_state);

    // 5. Clean up (currently does nothing, but good practice)
    // cleanup_game_state(&game_state);

    return 0;
}

// --- Function Implementations ---

static void print_usage(const char* prog_name) {
    printf("Usage: %s <scene_id>\n", prog_name);
    printf("Example: %s \"story/00_entry.md\"\n", prog_name);
}

static void debug_print_scene(const StoryScene* scene, const GameState* game_state) {
    if (scene == NULL) {
        printf("Scene is NULL.\n");
        return;
    }

    printf("Scene ID:         %s\n", scene->scene_id);
    printf("Scene Name:       %s\n", scene->name);
    printf("Location ID:      %s\n\n", scene->location_id);

    printf("--- Text Content (%d lines) ---\n", scene->text_line_count);
    for (int i = 0; i < scene->text_line_count; i++) {
        StringID id = scene->text_content_ids[i];
        printf("  [%d]: \"%s\"\n", id, get_string_by_id(id));
    }
    printf("\n");

    printf("--- Choices (%d choices) ---\n", scene->choice_count);
    for (int i = 0; i < scene->choice_count; i++) {
        const StoryChoice* choice = &scene->choices[i];
        printf("  Choice %d:\n", i + 1);
        printf("    Text:     \"%s\" (ID: %d)\n", get_string_by_id(choice->text_id), choice->text_id);
        printf("    Action:   %s\n", choice->action_id);
        if (choice->condition.flag_name[0] != '\0') {
            printf("    Condition: %s == %d\n", choice->condition.flag_name, choice->condition.required_value);
        }
    }
    printf("-------------------------------\n");
}
