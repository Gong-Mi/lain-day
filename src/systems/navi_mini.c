// This file implements the interactive NAVI mini system for Lain's desktop computer.
// Its content is based on the description in 01a_examine_navi.md.

#include "../include/systems/navi_mini.h"
#include "string_table.h"
#include "string_ids.h"      // Required for string IDs
#include "render_utils.h"
#include "linenoise.h"
#include "flag_system.h"
#include "ansi_colors.h"
#include "executor.h"       // Required to call execute_action
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// --- Constants ---
#define NAVI_PROMPT "NAVI> "
#define COLOR_NAVI_SYSTEM ANSI_COLOR_CYAN
#define COLOR_NAVI_ERROR ANSI_COLOR_RED
#define COLOR_NAVI_SUCCESS ANSI_COLOR_GREEN

// --- Helper Functions ---

static void get_next_navi_input(char* buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);

    if (g_argc > *g_arg_index_ptr) {
        strncpy(buffer, g_argv[*g_arg_index_ptr], buffer_size - 1);
        (*g_arg_index_ptr)++;
    } else {
        char *line = linenoise(NAVI_PROMPT);
        if (line != NULL) {
            if (strlen(line) > 0) {
                // linenoiseHistoryAdd(line);
            }
            strncpy(buffer, line, buffer_size - 1);
            free(line);
        } else {
            strncpy(buffer, "exit", buffer_size - 1);
        }
    }
}

static void print_header(GameState* game_state) {
    clear_screen();
    printf("%s========================================\n", COLOR_NAVI_SYSTEM);
    printf("   NAVI MINI (IWAKURA CUSTOM)\n");
    printf("========================================\n%s", ANSI_COLOR_RESET);
    
    // Simulate the "DATA ASSIMILATION" bar from navi.md
    // Ideally this should read from a game variable/flag
    int level = 1; // Default
    // int level = get_flag_value("level"); // Future integration
    
    printf("\nDATA ASSIMILATION: ");
    if (level <= 1)      printf("[=>....................] 5%%\n");
    else if (level == 2) printf("[=====>................] 28%%\n");
    else                 printf("[==============>.......] 77%%\n");

    // Network Signal Indicator
    const char* scope = hash_table_get(game_state->flags, "network_status.scope");
    const char* signal_indicator = "[----]";
    if (scope != NULL) {
        if (strcmp(scope, "地区局域网") == 0) {
            signal_indicator = "[||--]";
        } else if (strcmp(scope, "全国互联网") == 0) {
            signal_indicator = "[||||]";
        }
    }
    printf("SIGNAL: %s\n", signal_indicator);
    
    printf("\n");
}

static void print_menu() {
    printf("指令选项:\n");
    printf("  [1] %s\n", get_string_by_id(TEXT_CHOICE_NAVI_SHUTDOWN));
    printf("  [2] %s\n", get_string_by_id(TEXT_CHOICE_NAVI_REBOOT));
    printf("  [3] %s\n", get_string_by_id(TEXT_CHOICE_NAVI_CONNECT));
    printf("  [0] %s\n", "什么都不做");
    printf("\n");
}

// --- Main Interface Loop ---

void enter_navi_mini(GameState* game_state) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stdout, "DEBUG: Entering NAVI mini interface.\n");
    fflush(stdout);
#endif
    char line[MAX_LINE_LENGTH];
    int running = 1;

    // Boot animation
    clear_screen();
    printf("%sBooting NAVI mini...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    usleep(500000); // 0.5s
    
    while (running) {
        print_header(game_state);
        print_menu();

        get_next_navi_input(line, sizeof(line));

        // Parse command
        if (strcmp(line, "exit") == 0 || strcmp(line, "0") == 0 || strcmp(line, "quit") == 0) {
            running = 0;
        } else if (strcmp(line, "1") == 0) {
            execute_action("navi_shutdown", game_state);
            running = 0; // Exit after executing action
        } else if (strcmp(line, "2") == 0) {
            execute_action("navi_reboot", game_state);
            running = 0; // Exit after executing action
        } else if (strcmp(line, "3") == 0) {
            execute_action("navi_connect", game_state);
            running = 0; // Exit after executing action
        } else if (strlen(line) > 0) {
            printf("%sUnknown command: '%s'\n%s", COLOR_NAVI_ERROR, line, ANSI_COLOR_RESET);
            usleep(800000); // Wait a bit so user sees error
        }
    }
    
    printf("%sShutting down interface...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    usleep(300000);
    // Do not clear screen, so the main game can re-render over it.
#ifdef USE_DEBUG_LOGGING
    fprintf(stdout, "DEBUG: Exiting NAVI mini interface.\n");
    fflush(stdout);
#endif
}
