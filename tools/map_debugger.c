#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game_types.h"
#include "data_loader.h"
#include "map_loader.h"
#include "string_table.h"

// The data loaders require the globals defined in `game_state_global.c` to be available.
// They are included as part of the GAME_ENGINE_SOURCES in CMakeLists.txt.

int main(int argc, char* argv[]) {
    // Set the global command-line arguments for other modules that might use them.
    g_argc = argc;
    g_argv = argv;
    printf("--- Map Data Verification Tool ---\n\n");

    // Use the global game_state pointer, allocating memory for it
    game_state = malloc(sizeof(GameState));
    if (game_state == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for GameState.\n");
        return 1;
    }
    memset(game_state, 0, sizeof(GameState));

    if (!load_string_table()) {
        fprintf(stderr, "Error: Failed to load string table.\n");
        free(game_state);
        return 1;
    }

    // Pass the pointer directly
    if (!load_map_data("data/map", game_state)) {
        fprintf(stderr, "Error: Failed to load map data.\n");
        cleanup_game_state(game_state);
        free(game_state);
        return 1;
    }

    printf("Total locations registered: %d\n\n", game_state->location_count);

    for (int i = 0; i < game_state->location_count; i++) {
        Location* loc = &game_state->all_locations[i];
        if(loc == NULL || loc->id[0] == '\0') continue;

        printf("--- Location ---\n");
        printf(" ID:   %s\n", loc->id);
        printf(" Name: %s\n", loc->name);
        
        if (loc->connection_count > 0) {
            printf(" Connections (%d):\n", loc->connection_count);
            for (int j = 0; j < loc->connection_count; j++) {
                Connection* conn = &loc->connections[j];
                printf("   - via action '%s' -> leads to '%s'\n", conn->action_id, conn->target_location_id);
            }
        } else {
            printf(" Connections: (none)\n");
        }
        printf("\n");
    }
    
    cleanup_game_state(game_state);
    free(game_state);
    printf("--- End of Report ---\n");

    return 0;
}