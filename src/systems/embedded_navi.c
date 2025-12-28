// This file implements the self-contained, interactive embedded NAVI system
// for the mobile phone interface.

#include "../include/systems/embedded_navi.h"
#include "../include/systems/mail_system.h" // New include
#include "../include/systems/mystery_system.h" // Mystery App include
#include "../include/systems/navi_shell.h" // Shell include
#include "string_table.h"
#include "render_utils.h"
#include "linenoise.h"
#include "flag_system.h"
#include "ansi_colors.h" // Include ANSI color definitions
#include "logger.h"
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
    printf("  [4] mystery    - Wired Mysteries (App)\n");
    printf("  [5] shell      - Terminal Emulator\n");
    printf("  [0] exit       - Log out\n");
    printf("\n");
}

static void handle_mail(GameState* game_state) {
    Mailbox mailbox;
    mail_system_init(&mailbox);
    mail_system_load_emails(&mailbox, "world/home/lain/Maildir"); // Use relative path

    char mail_cmd_line[MAX_LINE_LENGTH];
    int mail_running = 1;

    while (mail_running) {
        clear_screen();
        print_header(game_state);
        mail_system_display_list(&mailbox);

        printf("%sMAIL> %s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
        get_next_navi_input(mail_cmd_line, sizeof(mail_cmd_line));

        if (strcmp(mail_cmd_line, "list") == 0) {
            // Already displayed by mail_system_display_list, just wait
            printf("%s(Already showing list. Type 'read <id>', 'delete <id>', or 'back')\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
            usleep(800000);
        } else if (strncmp(mail_cmd_line, "read ", 5) == 0) {
            int email_id_to_read = atoi(mail_cmd_line + 5);
            int email_index_to_read = -1;
            for (int i = 0; i < mailbox.email_count; ++i) {
                if (mailbox.emails[i].id == email_id_to_read && !mailbox.emails[i].is_deleted) { // Only read non-deleted
                    email_index_to_read = i;
                    break;
                }
            }

            if (email_index_to_read != -1) {
                clear_screen();
                print_header(game_state);
                mail_system_display_email(&mailbox, email_index_to_read);
                mail_system_mark_as_read(&mailbox, email_id_to_read);
                printf("\n(Press ENTER to return to mail list)");
                (void)getchar();
            } else {
                printf("%sERROR: Invalid or deleted email ID. Type 'list' to see available IDs.\n%s", COLOR_NAVI_ERROR, ANSI_COLOR_RESET);
                usleep(800000);
            }
        } else if (strncmp(mail_cmd_line, "delete ", 7) == 0) { // New delete command
            int email_id_to_delete = atoi(mail_cmd_line + 7);
            mail_system_delete_email(&mailbox, "world/home/lain/Maildir", email_id_to_delete);
            // After deletion, reload emails to reflect the change in the list
            mail_system_load_emails(&mailbox, "world/home/lain/Maildir");
            usleep(800000); // Give user time to read output
        } else if (strcmp(mail_cmd_line, "back") == 0 || strcmp(mail_cmd_line, "exit") == 0 || strcmp(mail_cmd_line, "0") == 0) {
            mail_running = 0;
        } else if (strlen(mail_cmd_line) > 0) {
            printf("%sUnknown mail command: '%s'. Try 'list', 'read <id>', 'delete <id>', or 'back'.\n%s", COLOR_NAVI_ERROR, mail_cmd_line, ANSI_COLOR_RESET);
            usleep(800000);
        }
    }

    mail_system_cleanup(&mailbox); // Clean up after exiting mail system
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
    printf("Hardware: NAVI-M (Mobile Model)\n");
    printf("Kernel:   Copland OS (Patched)\n");
    printf("User:     Iwakura Lain\n");
    printf("Uptime:   00:04:21\n");
    printf("(Press ENTER to return)");
    (void)getchar();
}

// --- Main Interface Loop ---

void enter_embedded_navi(GameState* game_state) {
    LOG_DEBUG("Entering Embedded NAVI interface.");
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
            LOG_DEBUG("NAVI command recognized: %s", line);
            running = 0;
        } else if (strcmp(line, "mail") == 0 || strcmp(line, "1") == 0) {
            LOG_DEBUG("NAVI command recognized: %s", line);
            handle_mail(game_state);
        } else if (strcmp(line, "net") == 0 || strcmp(line, "2") == 0) {
            LOG_DEBUG("NAVI command recognized: %s", line);
            handle_network(game_state);
        } else if (strcmp(line, "sys") == 0 || strcmp(line, "3") == 0) {
            LOG_DEBUG("NAVI command recognized: %s", line);
            handle_system_info();
        } else if (strcmp(line, "mystery") == 0 || strcmp(line, "4") == 0) {
            enter_mystery_app(game_state);
        } else if (strcmp(line, "shell") == 0 || strcmp(line, "5") == 0) {
            enter_navi_shell(game_state);
        } else if (strlen(line) > 0) {
            printf("%sUnknown command: '%s'\n%s", COLOR_NAVI_ERROR, line, ANSI_COLOR_RESET);
            usleep(800000); // Wait a bit so user sees error
        }
    }
    
    printf("%sShutting down interface...\n%s", COLOR_NAVI_SYSTEM, ANSI_COLOR_RESET);
    usleep(300000);
    clear_screen();
    LOG_DEBUG("Exiting Embedded NAVI interface.");
}
