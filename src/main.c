#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <libgen.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>
#include <locale.h> // Required for setlocale()
#include <signal.h> // For signal handling
#include <pthread.h>

#include "game_types.h"
#include "game_paths.h"
#include "scenes.h"
#include "event_system.h"
#include "time_utils.h"
#include "render_utils.h"
#include "string_table.h"
#include "data_loader.h"
#include "map_loader.h"
#include "executor.h"
#include "characters/mika.h"
#include "ecc_time.h"
#include "project_status.h"
#include "build_info.h"
#include "linenoise.h"
#include "cmap.h"
#include "flag_system.h"
#include "logo_raw_data.h"
#include "character_data.h"
#include "ansi_colors.h"

// --- Global State ---
static uint32_t scene_entry_time = 0;
volatile sig_atomic_t g_needs_redraw = 0;

// --- Function Prototypes ---
void handle_signal(int sig);
void handle_sigwinch(int sig);
void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index);
int is_numeric(const char* str);
bool is_valid_session_name(const char* name); // Added prototype for session name validation
static bool process_events(GameState* game_state, StoryScene* current_scene);

// --- Signal Handling ---
void handle_sigwinch(int sig) {
    (void)sig;
    g_needs_redraw = 1;
}

void handle_signal(int sig) {
    (void)sig; // Suppress unused variable warning
    restore_terminal_state();
    printf("\nLain-day C version terminated by signal.\n");
    exit(0);
}

// --- Event Handling ---
static bool process_events(GameState* game_state, StoryScene* current_scene) {
    Event e;
    bool scene_has_changed = false;
    while (poll_event(&e)) {
        if (e.type == TIME_TICK_EVENT) {
            mika_update_location_by_schedule(game_state);
            if (strcmp(current_scene->scene_id, "SCENE_00_ENTRY") == 0) {
                DecodedTimeResult current_time_decoded = decode_time_with_ecc(game_state->time_of_day);
                if (current_time_decoded.status != DOUBLE_BIT_ERROR_DETECTED) {
                    if ((current_time_decoded.data - scene_entry_time) >= (60 * 16)) {
                        const char* flag_val = hash_table_get(game_state->flags, "door_opened_by_ghost");
                        if (flag_val == NULL || strcmp(flag_val, "1") != 0) {
                            hash_table_set(game_state->flags, "door_opened_by_ghost", "1");
                            strncpy(game_state->current_story_file, "SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE", MAX_PATH_LENGTH - 1);
                            scene_has_changed = true;
                        }
                    }
                }
            }
        }
        if (scene_has_changed) break;
    }
    return scene_has_changed;
}

// --- Main Function ---
int main(int argc, char *argv[]) {
    setlocale(LC_ALL, ""); // Set locale for proper multibyte character handling
    g_argc = argc;
    g_argv = argv;
    init_mika_module();
    pthread_t time_thread_id;
    pthread_mutex_init(&time_mutex, NULL);
    init_event_queue();

    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Use sigaction for SIGWINCH to ensure SA_RESTART is NOT set,
    // so that read() is interrupted and we can redraw immediately.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sa.sa_flags = 0; // Explicitly NO SA_RESTART
    sigaction(SIGWINCH, &sa, NULL);

    printf("Lain-day C version starting...\n");
    printf("Build Info - OS: %s, Arch: %s\n", BUILD_OS, BUILD_ARCH);

    GamePaths paths;
    init_paths(argv[0], &paths);

    char session_name[MAX_NAME_LENGTH] = {0};
    char character_session_file_path[MAX_PATH_LENGTH] = {0};
    char session_dir_path[MAX_PATH_LENGTH] = {0};
    int arg_index = 1;
    g_arg_index_ptr = &arg_index;
    bool is_test_mode = false;

    // Allocate and initialize the global game_state pointer
    game_state = malloc(sizeof(GameState));
    if (game_state == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for GameState.\n");
        return 1;
    }
    memset(game_state, 0, sizeof(GameState));
    
    // Initialize Doll States (Default)
    game_state->doll_state_lain_room = DOLL_STATE_NORMAL;
    game_state->doll_state_mika_room = DOLL_STATE_NORMAL;

    // Copy initialized paths to game_state
    memcpy(&game_state->paths, &paths, sizeof(GamePaths));

    // Load string table first as other components depend on it
    if (!load_string_table()) {
        fprintf(stderr, "Error: Failed to load the string table.\n");
        free(game_state);
        return 1;
    }
    // Copy initialized paths to game_state
    memcpy(&game_state->paths, &paths, sizeof(GamePaths));

    // Load string table first as other components depend on it
    if (!load_string_table()) {
        fprintf(stderr, "Error: Failed to load the string table.\n");
        free(game_state);
        return 1;
    }

    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-d") == 0) {
            is_test_mode = true;
            printf("Running in test mode (-d). Temporary session.\n");
        } else {
            fprintf(stderr, "Warning: Unknown argument '%s'. Ignoring.\n", argv[arg_index]);
        }
        arg_index++;
    }

    if (!is_test_mode && arg_index >= argc) {
        enter_fullscreen_mode();
        ImageBounds bounds = render_image_adaptively(LOGO_DATA, LOGO_WIDTH, LOGO_HEIGHT);
        printf("\n\nPress any key or click the image to start...");
        fflush(stdout);
        
        enable_raw_mode();
        
        char c;
        int state = 0; // 0: start, 1: esc, 2: csi, 3: mouse
        char mouse_buf[32];
        int mouse_idx = 0;

        while (1) {
            if (g_needs_redraw) {
                g_needs_redraw = 0;
                clear_screen();
                bounds = render_image_adaptively(LOGO_DATA, LOGO_WIDTH, LOGO_HEIGHT);
                printf("\n\nPress any key or click the image to start...");
                fflush(stdout);
            }

            ssize_t n = read(STDIN_FILENO, &c, 1);
            
            if (n == -1) {
                if (errno == EINTR) {
                    continue; // Loop back to check g_needs_redraw
                }
                break; // Real error
            }
            if (n == 0) break; // EOF (e.g. pipe closed)

            if (state == 0) {
                if (c == '\x1b') {
                    state = 1;
                } else {
                    break; // Any regular key
                }
            } else if (state == 1) {
                if (c == '[') {
                    state = 2;
                } else {
                    break; // Alt+Key or other escape sequence
                }
            } else if (state == 2) {
                if (c == '<') {
                    state = 3;
                    mouse_idx = 0;
                } else {
                    break; // Other CSI sequence (e.g. arrow keys)
                }
            } else if (state == 3) {
                if (c == 'M' || c == 'm') {
                    mouse_buf[mouse_idx] = '\0';
                    int btn, x, y;
                    if (sscanf(mouse_buf, "%d;%d;%d", &btn, &x, &y) == 3) {
                        // Check for Left Click (btn 0) Press (M)
                        if (c == 'M' && btn == 0) {
                             if (x >= bounds.start_x && x <= bounds.end_x &&
                                 y >= bounds.start_y && y <= bounds.end_y) {
                                 break; // Valid click on logo
                             }
                        }
                    }
                    state = 0; // Reset if invalid click or release
                } else {
                    if (mouse_idx < (int)sizeof(mouse_buf) - 1) {
                        mouse_buf[mouse_idx++] = c;
                    }
                }
            }
        }

        disable_raw_mode();
        // Give a tiny moment for any pending input (like mouse release) to arrive before flushing
        usleep(50000); 
        flush_input_buffer();
        clear_screen(); // Clear logo before showing session prompt

        // Switch SIGWINCH to SA_RESTART for the rest of the game.
        struct sigaction sa_restart;
        memset(&sa_restart, 0, sizeof(sa_restart));
        sa_restart.sa_handler = handle_sigwinch;
        sa_restart.sa_flags = SA_RESTART; 
        sigaction(SIGWINCH, &sa_restart, NULL);

        // --- Intro Sequence ---
        // Turn off echo so user can type their name ahead of time, but it won't 
        // mess up the cinematic with characters like "^[[A".
        set_terminal_echo(false);

        printf("\n\n");
        printf(ANSI_COLOR_CYAN "   CLOSE THE WORLD,\n" ANSI_COLOR_RESET);
        usleep(600000);
        printf(ANSI_COLOR_MAGENTA "           OPEN THE NExT.\n" ANSI_COLOR_RESET);
        usleep(800000);
        printf("\n");
        
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_YELLOW " WARN " ANSI_COLOR_BRIGHT_BLACK "]   NO TRUSTED ENVIRONMENT DETECTED.\n" ANSI_COLOR_RESET);
        usleep(400000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_GREEN " OK " ANSI_COLOR_BRIGHT_BLACK "]     SYSTEM INTEGRITY CHECK COMPLETE.\n" ANSI_COLOR_RESET);
        usleep(300000);
        
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_CYAN " SYSTEM " ANSI_COLOR_BRIGHT_BLACK "] INITIALIZING PROTOCOLS...\n" ANSI_COLOR_RESET);
        usleep(300000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_CYAN " NET " ANSI_COLOR_BRIGHT_BLACK "]    CONNECTING TO DEFAULT GATEWAY...\n" ANSI_COLOR_RESET);
        usleep(600000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_RED " ERROR " ANSI_COLOR_BRIGHT_BLACK "]  CONNECTION REFUSED (TIMEOUT).\n" ANSI_COLOR_RESET);
        usleep(400000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_YELLOW " WARN " ANSI_COLOR_BRIGHT_BLACK "]   REROUTING VIA BACKUP GATEWAY (IPv4)...\n" ANSI_COLOR_RESET);
        usleep(500000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_CYAN " NET " ANSI_COLOR_BRIGHT_BLACK "]    HANDSHAKE INITIATED...\n" ANSI_COLOR_RESET);
        usleep(300000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_GREEN " OK " ANSI_COLOR_BRIGHT_BLACK "]     CONNECTION ESTABLISHED.\n" ANSI_COLOR_RESET);
        usleep(300000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_RED " WARN " ANSI_COLOR_BRIGHT_BLACK "]   TARGET_IDENTITY IS NULL (null).\n" ANSI_COLOR_RESET);
        usleep(400000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_YELLOW " WARN " ANSI_COLOR_BRIGHT_BLACK "]   ATTEMPTING SESSION RECOVERY...\n" ANSI_COLOR_RESET);
        usleep(600000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_RED " ERROR " ANSI_COLOR_BRIGHT_BLACK "]  NO ARCHIVE HISTORY FOUND.\n" ANSI_COLOR_RESET);
        usleep(400000);
        printf(ANSI_COLOR_BRIGHT_BLACK "   [" ANSI_COLOR_YELLOW " WARN " ANSI_COLOR_BRIGHT_BLACK "]   INITIALIZING WORKSPACE...\n" ANSI_COLOR_RESET);
        usleep(300000);
        printf("\n");
        
        set_terminal_echo(true); // Turn echo back on for session input
        // ----------------------


    }

    if (is_test_mode) {
        printf("Test mode enabled. Using default character state without session persistence.\n");
        snprintf(character_session_file_path, MAX_PATH_LENGTH, "test_char.json"); 
        if (access(character_session_file_path, F_OK) != -1) {
             if(remove(character_session_file_path) != 0) {
                perror("Error deleting existing test_char.json");
             }
        }
        if (!write_string_to_file(CHARACTER_JSON_DATA, character_session_file_path)) {
            fprintf(stderr, "Error: Failed to write embedded character data to temp file for test mode.\n");
            return 1;
        }
    } else {
        if (argc > arg_index) {
            strncpy(session_name, argv[arg_index], MAX_NAME_LENGTH - 1);
            session_name[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
            if (!is_valid_session_name(session_name)) {
                printf("%s: %s\n", get_string_by_id(TEXT_ERROR_INVALID_SESSION_NAME), session_name);
                return 1; // Exit on invalid argument
            }
            printf("Using session name from argument: %s\n", session_name);
            arg_index++;
        } else {
            int session_error_count = 0;
            while (session_name[0] == '\0') {
                char* line = linenoise(get_string_by_id(TEXT_PROMPT_SESSION_NAME));
                
                if (line != NULL) {
                    strncpy(session_name, line, MAX_NAME_LENGTH - 1);
                    session_name[MAX_NAME_LENGTH - 1] = '\0';
                    free(line);

                    if (strlen(session_name) == 0) {
                        session_error_count++;
                        if (session_error_count > 1) {
                            // Move up 2 lines: 1 for the prompt, 1 for the previous error
                            printf("\033[2A\r\033[K");
                        } else {
                            // First error: just move up 1 line to cover the prompt
                            printf("\033[A\r\033[K");
                        }
                        printf(ANSI_COLOR_RED "%s (Errors: %d)" ANSI_COLOR_RESET "\n", 
                               get_string_by_id(TEXT_ERROR_SESSION_NAME_EMPTY), session_error_count);
                        printf("\r\033[K"); // Clear the line where the new prompt will appear
                        fflush(stdout);
                        continue;
                    }
                    if (!is_valid_session_name(session_name)) {
                        session_error_count++;
                        if (session_error_count > 1) {
                            printf("\033[2A\r\033[K");
                        } else {
                            printf("\033[A\r\033[K");
                        }
                        printf(ANSI_COLOR_RED "%s (Errors: %d)" ANSI_COLOR_RESET "\n", 
                               get_string_by_id(TEXT_ERROR_INVALID_SESSION_NAME), session_error_count);
                        printf("\r\033[K");
                        fflush(stdout);
                        session_name[0] = '\0';
                        continue;
                    }
                } else {
                    // Real EOF or error (not interrupted by signal anymore due to SA_RESTART)
                    fprintf(stderr, "Error: Failed to read session name.\n");
                    return 1;
                }
            }
        }
        
            // No need for separate empty check here, handled in loop
            // if (strlen(session_name) == 0) {
            //     fprintf(stderr, "Error: Session name cannot be empty.\n");
            //     return 1;
            // }
        if (!ensure_directory_exists_recursive(paths.session_root_dir, 0755)) {
            fprintf(stderr, "Error: Failed to create session root directory '%s'. Check permissions or path.\n", paths.session_root_dir);
            return 1;
        }
        snprintf(session_dir_path, MAX_PATH_LENGTH, "%s/%s", paths.session_root_dir, session_name);
        if (!ensure_directory_exists_recursive(session_dir_path, 0755)) {
            fprintf(stderr, "Error: Failed to create session directory '%s'. Check permissions or path.\n", session_dir_path);
            return 1;
        }
        snprintf(character_session_file_path, MAX_PATH_LENGTH, "%s/character.json", session_dir_path);
        if (access(character_session_file_path, F_OK) == -1) {
            printf(get_string_by_id(TEXT_SESSION_NEW_MESSAGE), session_name);
            if (!write_string_to_file(CHARACTER_JSON_DATA, character_session_file_path)) {
                fprintf(stderr, "Error: Failed to write session file.\n");
                return 1;
            }
        } else {
            printf("Resuming session '%s'.\n", session_name);
        }
    }

    if (!load_player_state(character_session_file_path, game_state)) {
        fprintf(stderr, "Error: Failed to load player state from '%s'.\n", character_session_file_path);
        return 1;
    }
    printf("Player data loaded.\n");

    if (!load_items_data(game_state)) return 1;
    printf("Items data loaded.\n");

    if (!load_map_data(NULL, game_state)) {
        fprintf(stderr, "Error: Failed to load map data.\n");
        return 1;
    }
    printf("Map data loaded.\n");

    pthread_create(&time_thread_id, NULL, time_thread_func, (void*)game_state);

    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;
    bool dirty = true;

    pthread_mutex_lock(&time_mutex);
    if (!transition_to_scene(game_state->current_story_file, &current_scene, game_state)) {
        fprintf(stderr, "Failed to initialize starting scene: %s\n", game_state->current_story_file);
        pthread_mutex_unlock(&time_mutex);
        return 1;
    }
    scene_entry_time = decode_time_with_ecc(game_state->time_of_day).data;
    pthread_mutex_unlock(&time_mutex);

    while (game_is_running) {
        get_next_input(input_buffer, sizeof(input_buffer), argc, argv, &arg_index);

        pthread_mutex_lock(&time_mutex);

        if (g_needs_redraw) {
            dirty = true;
            g_needs_redraw = 0;
        }

        if (process_events(game_state, &current_scene)) {
            dirty = true;
        }

        if (input_buffer[0] != '\0') {
            if (strcmp(input_buffer, "quit") == 0) {
                game_is_running = false;
            } else if (is_numeric(input_buffer)) {
                int choice_num = atoi(input_buffer);
                int visible_choice_count = 0;
                int target_array_index = -1;
                for (int i = 0; i < current_scene.choice_count; i++) {
                    if (is_choice_selectable(&current_scene.choices[i], game_state)) {
                        visible_choice_count++;
                        if (visible_choice_count == choice_num) {
                            target_array_index = i;
                            break;
                        }
                    }
                }
                if (target_array_index != -1) {
                    if (execute_action(current_scene.choices[target_array_index].action_id, game_state)) {
                        dirty = true;
                    }
                } else {
                    printf("Invalid choice.\n");
                }
            } else {
                if (execute_command(input_buffer, game_state)) {
                    dirty = true;
                }
            }
            if (game_is_running) dirty = true;
        }

        if (dirty) {
            if (strcmp(current_scene.scene_id, game_state->current_story_file) != 0) {
                if (transition_to_scene(game_state->current_story_file, &current_scene, game_state)) {
                    scene_entry_time = decode_time_with_ecc(game_state->time_of_day).data;
                } else {
                    fprintf(stderr, "CRITICAL ERROR: Failed to transition to scene '%s'. Scene ID not found or invalid. Terminating game to prevent undefined state.\n", game_state->current_story_file);
                    game_is_running = false;
                }
            }
            if (game_is_running) {
                render_current_scene(&current_scene, game_state);
            }
            dirty = false;
        }

        pthread_mutex_unlock(&time_mutex);
        usleep(50000);
    }
    
    pthread_join(time_thread_id, NULL);
    
    if (!is_test_mode) {
        if (!save_game_state(character_session_file_path, game_state)) {
             fprintf(stderr, "Error saving game state.\n");
        }
    }
    
    cleanup_game_state(game_state);
    pthread_mutex_destroy(&time_mutex);

    restore_terminal_state();
    printf("\nLain-day C version exiting.\n");
    return 0;
}

void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index) {
    memset(buffer, 0, buffer_size);

    if (argc > *arg_index) {
        strncpy(buffer, argv[*arg_index], buffer_size - 1);
        (*arg_index)++;
    } else {
        fd_set readfds;
        struct timeval tv;
        int retval;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (retval > 0) {
            errno = 0;
            char *line = linenoise(get_string_by_id(TEXT_PROMPT_INPUT_ARROW));
            if (line != NULL) {
                if (strlen(line) > 0) {
                    // linenoiseHistoryAdd(line); // Temporarily disable history to debug segfault
                }
                strncpy(buffer, line, buffer_size - 1);
                free(line);
            } else {
                if (errno == EINTR) {
                    buffer[0] = '\0'; // Not a quit, just interrupted
                } else {
                    strncpy(buffer, "quit", buffer_size - 1);
                }
            }
        }
    }
}

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

bool is_valid_session_name(const char* name) {
    if (name == NULL || *name == '\0') {
        return false;
    }
    for (int i = 0; name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];
        // Allow alphanumeric, underscore, hyphen, and UTF-8 multi-byte characters
        if (!isalnum(c) && c != '_' && c != '-' && c < 128) {
            return false; // Found an invalid character
        }
    }
    return true; // All characters are valid
}