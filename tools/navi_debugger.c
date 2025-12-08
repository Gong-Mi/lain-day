#include <stdio.h>
#include <string.h>
#include "game_types.h"
#include "string_table.h"
#include "flag_system.h"
#include "data_loader.h"
#include "systems/embedded_navi.h"

int main(int argc, char* argv[]) {
    g_argc = argc;
    g_argv = argv;
    int arg_index = 1;
    g_arg_index_ptr = &arg_index;

    // 1. Initialize a minimal GameState
    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));
    g_game_state_ptr = &game_state;

    // 2. Initialize necessary systems
    if (!load_string_table()) {
        fprintf(stderr, "ERROR: Failed to load string table for NAVI debugger.\n");
        return 1;
    }
    
    game_state.flags = create_hash_table(MAX_FLAGS);
    if (game_state.flags == NULL) {
        fprintf(stderr, "ERROR: Failed to create flags hash table.\n");
        cleanup_string_table();
        return 1;
    }

    // Set a default network scope for testing
    hash_table_set(game_state.flags, "network_status.scope", "地区局域网");

    // 3. Enter the NAVI interface
    // This function contains its own interactive loop.
    enter_embedded_navi(&game_state);

    // 4. Cleanup
    free_hash_table(game_state.flags);
    cleanup_string_table();

    printf("NAVI Debugger exited.\n");
    return 0;
}
