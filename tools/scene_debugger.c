#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For atoi, exit
#include "game_types.h"
#include "scenes.h"
#include "data_loader.h"
#include "string_table.h"
#include "map_loader.h" // Required for map loading
#include "cmap.h" // Required for cmap_destroy

// --- Forward Declarations ---
static void print_usage(const char* prog_name);
static void debug_print_scene(const StoryScene* scene, const GameState* game_state);
static void debug_print_location(const Location* loc);

// --- Main Function ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* target_id = NULL;
    bool debug_location = false;
    int arg_offset = 1; // Used to track current argument position

#ifdef USE_MAP_DEBUG_LOGGING
    if (argc >= 3 && strcmp(argv[1], "-l") == 0) {
        debug_location = true;
        target_id = argv[2];
        arg_offset = 3;
    } else {
        target_id = argv[1];
    }
#else
    target_id = argv[1];
#endif

    // Basic argument count check based on mode
    if (debug_location && argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    if (!debug_location && argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    // 1. Initialize a minimal GameState
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));
    g_game_state_ptr = &game_state; // Set the global pointer for the debugger instance

    // Initialize paths
    GamePaths paths;
    init_paths(argv[0], &paths);

    // Load string table
    char strings_json_path[MAX_PATH_LENGTH];
    snprintf(strings_json_path, MAX_PATH_LENGTH, "%s/data/strings.json", paths.base_path);
    if (!load_string_table(strings_json_path)) {
        fprintf(stderr, "ERROR: Failed to load string table for scene debugger.\n");
        return 1;
    }

    // 2. Load game data
    // For locations, we need to load the full map data.
    // For scenes, only string tables are generally needed, but loading map data provides context.
    // We mock player_state.location as "start" for map loading to proceed.
    strncpy(game_state.player_state.location, "start", MAX_NAME_LENGTH - 1); // Mock location ID
    game_state.player_state.location[MAX_NAME_LENGTH - 1] = '\0';


    if (debug_location) {
        printf("--- Debugging Location: %s ---\n\n", target_id);

        GamePaths paths;
        // Mock path for map_dir, since load_map_data ignores it anyway for programmatic loading
        strncpy(paths.map_dir, ".", MAX_PATH_LENGTH - 1); 

        if (!load_map_data(paths.map_dir, &game_state)) {
            fprintf(stderr, "ERROR: Failed to load map data for location debugging.\n");
            return 1;
        }

        const Location* loc = (const Location*)cmap_get(game_state.location_map, target_id);
        if (loc == NULL) {
            fprintf(stderr, "ERROR: Location '%s' not found in map data.\n", target_id);
            cmap_destroy(game_state.location_map);
            return 1;
        }
        debug_print_location(loc);
        cmap_destroy(game_state.location_map); // Clean up map
    } else { // Debugging a scene
        printf("--- Debugging Scene: %s ---\n\n", target_id);
        StoryScene scene;
        if (!transition_to_scene(target_id, &scene, &game_state)) {
            fprintf(stderr, "ERROR: Failed to transition to scene '%s'. Is it registered in scenes.c?\n", target_id);
            return 1;
        }
        debug_print_scene(&scene, &game_state);
    }
    
    cleanup_string_table(); // Clean up string table
    return 0;
}

// --- Function Implementations ---

static void print_usage(const char* prog_name) {
    printf("Usage:\n");
    printf("  Scene Debugger: %s <scene_id>\n", prog_name);
    printf("    Example: %s \"story/00_entry.md\"\n", prog_name);
#ifdef USE_MAP_DEBUG_LOGGING
    printf("  Location Debugger: %s -l <location_id>\n", prog_name);
    printf("    Example: %s -l \"cyberia_club\"\n", prog_name);
#endif
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
        if (choice->condition.flag_name[0] != '\0') {
            printf("    Condition: %s == %d\n", choice->condition.flag_name, choice->condition.required_value);
        }
    }
    printf("-------------------------------\n");
}

static void debug_print_location(const Location* loc) {
    if (loc == NULL) {
        printf("Location is NULL.\n");
        return;
    }

    printf("Location ID:   %s\n", loc->id);
    printf("Location Name: %s\n", loc->name);
    printf("Description:   %s\n", loc->description);

    printf("\n--- Points of Interest (%d POIs) ---\n", loc->pois_count);
    if (loc->pois_count == 0) {
        printf("  (none)\n");
    } else {
        for (int i = 0; i < loc->pois_count; i++) {
            printf("  POI %d ID:          %s\n", i + 1, loc->pois[i].id);
            printf("    Name:        %s\n", loc->pois[i].name);
            printf("    Description: %s\n", loc->pois[i].description);
        }
    }

    printf("\n--- Connections (%d connections) ---\n", loc->connection_count);
    if (loc->connection_count == 0) {
        printf("  (none)\n");
    } else {
        for (int i = 0; i < loc->connection_count; i++) {
            const Connection* conn = &loc->connections[i];
            printf("  Connection %d Action: %s\n", i + 1, conn->action_id);
            printf("    Target:    %s\n", conn->target_location_id);
            if (conn->is_accessible != NULL) {
                printf("    Access Check: (function pointer)\n");
            }
            if (conn->access_denied_scene_id != NULL) {
                printf("    Access Denied Scene: %s\n", conn->access_denied_scene_id);
            }
        }
    }
    printf("-------------------------------\n");
}
