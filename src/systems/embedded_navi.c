#include "../include/systems/embedded_navi.h"
#include "string_table.h"
#include "render_utils.h"
#include "linenoise.h"
#include "flag_system.h"
#include "ansi_colors.h" // Include ANSI color definitions
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // for sleep/usleep

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
    printf("   NAVI EMBEDDED SYSTEM v0.1a (UNREGISTERED)\n");
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
    printf("Available Modules:\n");
    printf("  [1] mail       - Check Emails\n");
    printf("  [2] net        - Network Status / Connect\n");
    printf("  [3] sys        - System Information\n");
    printf("  [0] exit       - Log out\n");
    printf("\n");
}

static void handle_mail(GameState* game_state) {
    printf("%s\nScanning mailboxes...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    sleep(1);
    printf("No new messages.\n");
    printf("(Press ENTER to return)");
    (void)getchar();
}

static void handle_network(GameState* game_state) {
    printf("%s\nChecking connection to The Wired...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    // TODO: Check actual game flag like FLAG_NAVI_ONLINE
    sleep(1);

    // Simulate connection progress
    const char* progress_frames[] = {"", "！", "！-", "！-=", "！-=≡"};
    printf("Connecting...");
    for (int i = 0; i < 5; ++i) {
        printf("\rConnecting... %s", progress_frames[i]); // \r returns cursor to start of line
        fflush(stdout); // Ensure it prints immediately
        usleep(200000); // 0.2 seconds delay
    }
    printf("\n"); // Newline after progress

    // Display final status based on scope
    const char* scope = hash_table_get(game_state->flags, "network_status.scope");
    const char* signal_strength_display = "[----]"; // Default no signal
    if (scope != NULL) {
        if (strcmp(scope, "地区局域网") == 0) {
            signal_strength_display = "[||--]"; // 2 bars for regional
        } else if (strcmp(scope, "全国互联网") == 0) {
            signal_strength_display = "[||||]"; // 4 bars for national
        }
    }
    
    // Check actual connection status (assuming flags determine this)
    // For now, let's assume if scope is not NULL, we are connected.
    // If we want a proper OFFLINE, we need a specific flag for it.
    if (scope != NULL) {
        printf("Status: %sCONNECTED%s %s\n", COLOR_NAVI_SUCCESS, ANSI_COLOR_RESET, signal_strength_display);
    } else {
        printf("Status: %sOFFLINE%s %s\n", COLOR_NAVI_ERROR, ANSI_COLOR_RESET, signal_strength_display);
    }

    printf("Gateway not found.\n"); 
    printf("(Press ENTER to return)");
    (void)getchar();
}

static void handle_system_info() {
    printf("%s\n--- SYSTEM INFO ---\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    printf("Hardware: NAVI-X (Desktop Model)\n");
    printf("Kernel:   Copland OS (Patched)\n");
    printf("User:     Iwakura Lain\n");
    printf("Uptime:   00:04:21\n");
    printf("(Press ENTER to return)");
    (void)getchar();
}

// --- Main Interface Loop ---

void enter_embedded_navi(GameState* game_state) {
#ifdef USE_DEBUG_LOGGING
    fprintf(stdout, "DEBUG: Entering Embedded NAVI interface.\n");
    fflush(stdout);
#endif
    char line[MAX_LINE_LENGTH];
    int running = 1;

    // Boot animation
    clear_screen();
    printf("%sBooting Embedded Interface...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    usleep(500000); // 0.5s
    
    while (running) {
        print_header(game_state);
        print_menu();

        get_next_navi_input(line, sizeof(line));

        // Parse command
        if (strcmp(line, "exit") == 0 || strcmp(line, "0") == 0 || strcmp(line, "quit") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stdout, "DEBUG: NAVI command recognized: %s\n", line);
            fflush(stdout);
#endif
            running = 0;
        } else if (strcmp(line, "mail") == 0 || strcmp(line, "1") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stdout, "DEBUG: NAVI command recognized: %s\n", line);
            fflush(stdout);
#endif
            handle_mail(game_state);
        } else if (strcmp(line, "net") == 0 || strcmp(line, "2") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stdout, "DEBUG: NAVI command recognized: %s\n", line);
            fflush(stdout);
#endif
            handle_network(game_state);
        } else if (strcmp(line, "sys") == 0 || strcmp(line, "3") == 0) {
#ifdef USE_DEBUG_LOGGING
            fprintf(stdout, "DEBUG: NAVI command recognized: %s\n", line);
            fflush(stdout);
#endif
            handle_system_info();
        } else if (strlen(line) > 0) {
            printf("%sUnknown command: '%s'\n%s", COLOR_NAVI_ERROR, line, ANSI_COLOR_RESET);
            usleep(800000); // Wait a bit so user sees error
        }
    }
    
    printf("%sShutting down interface...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    usleep(300000);
    clear_screen();
#ifdef USE_DEBUG_LOGGING
    fprintf(stdout, "DEBUG: Exiting Embedded NAVI interface.\n");
    fflush(stdout);
#endif
}
