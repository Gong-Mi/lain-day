#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atoi, exit
#include "game_types.h"
#include "scenes.h"
#include "data_loader.h"
#include "string_table.h"
#include "map_loader.h" // Required for map loading
#include "cmap.h" // Required for cmap_destroy
#include "game_paths.h"

// --- Forward Declarations ---
static void print_usage(const char* prog_name);
static void debug_print_scene(const StoryScene* scene, const GameState* game_state);

// --- Main Function ---
int main(int argc, char* argv[]) {
    g_argc = argc;
    g_argv = argv;
    int arg_index = 1;
    g_arg_index_ptr = &arg_index;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* target_id = argv[1];

    // 1. Initialize a minimal GameState
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));
    g_game_state_ptr = &game_state; // Set the global pointer for the debugger instance

    // Load string table from embedded data
    if (!load_string_table()) {
        fprintf(stderr, "ERROR: Failed to load string table for scene debugger.\n");
        return 1;
    }

    // 2. Load game data
    printf("--- Debugging Scene: %s ---\n\n", target_id);
    StoryScene scene;
    if (!transition_to_scene(target_id, &scene, &game_state)) {
        fprintf(stderr, "ERROR: Failed to transition to scene '%s'. Is it registered in scenes.c?\n", target_id);
        return 1;
    }
    debug_print_scene(&scene, &game_state);
    
    cleanup_string_table(); // Clean up string table
    return 0;
}

// --- Function Implementations ---

static void print_usage(const char* prog_name) {
    printf("Usage:\n");
    printf("  Scene Debugger: %s <scene_id>\n", prog_name);
    printf("    Example: %s \"SCENE_00_ENTRY\"\n", prog_name);
}

static const char* get_speaker_name_str(SpeakerID id) {
    switch (id) {
        case SPEAKER_NONE: return "NONE";
        case SPEAKER_LAIN: return "Lain (你)";
        case SPEAKER_MOM: return "妈妈";
        case SPEAKER_DAD: return "爸爸";
        case SPEAKER_ALICE: return "Alice";
        case SPEAKER_CHISA: return "Chisa";
        case SPEAKER_MIRA: return "Mika (姐姐)";
        case SPEAKER_GHOST: return "幽灵";
        case SPEAKER_DOCTOR: return "冬子老师";
        case SPEAKER_NAVI: return "Navi";
        case SPEAKER_PARENT: return "父母";
        default: return "UNKNOWN";
    }
}

static void debug_print_scene(const StoryScene* scene, const GameState* game_state) {
    if (scene == NULL) {
        printf("Scene is NULL.\n");
        return;
    }

    printf("Scene ID:         %s\n", scene->scene_id);
    printf("Scene Name:       %s\n", scene->name);
    printf("Location ID:      %s\n\n", scene->location_id);

    printf("--- Text Content (%d lines) ---\n", scene->dialogue_line_count);
    for (int i = 0; i < scene->dialogue_line_count; i++) {
        const DialogueLine* line = &scene->dialogue_lines[i];
        printf("  [Line %d]: SpeakerID=%d (%s), StringID=%d, Text=\"%s\"\n",
               i, line->speaker_id, get_speaker_name_str(line->speaker_id),
               line->text_id, get_string_by_id(line->text_id));
    }
    printf("\n");

    printf("--- Choices (%d choices) ---\n", scene->choice_count);
    for (int i = 0; i < scene->choice_count; i++) {
        const StoryChoice* choice = &scene->choices[i];
        printf("  Choice %d:\n", i + 1);
        printf("    Text:     \"%s\" (ID: %d)\n", get_string_by_id(choice->text_id), choice->text_id);
        printf("    Action:   %s\n", choice->action_id);

        if (choice->condition_count > 0) {
            printf("    Conditions (%d):\n", choice->condition_count);
            for (int j = 0; j < choice->condition_count; j++) {
                const Condition* cond = &choice->conditions[j];
                printf("      - Condition %d:\n", j + 1);
                if (cond->flag_name[0] != '\0') {
                    if (cond->required_value[0] != '\0') {
                        printf("          Flag '%s' must be '%s'\n", cond->flag_name, cond->required_value);
                    } else {
                        printf("          Flag '%s' must be set\n", cond->flag_name);
                    }
                }
                if (cond->exact_day != -1) {
                    printf("          Must be exactly day %d\n", cond->exact_day);
                }
                if (cond->min_day != -1) {
                    printf("          Must be on or after day %d\n", cond->min_day);
                }
                if (cond->max_day != -1) {
                    printf("          Must be on or before day %d\n", cond->max_day);
                }
                if (cond->hour_start != -1) {
                    printf("          Hour must be >= %d\n", cond->hour_start);
                }
                if (cond->hour_end != -1) {
                    printf("          Hour must be <= %d\n", cond->hour_end);
                }
            }
        }
    }
    printf("-------------------------------\n");
}
