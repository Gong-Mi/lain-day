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

#include "build_info.h"
#include "game_types.h"
#include "data_loader.h"
#include "map_loader.h"
#include "executor.h"
#include "string_table.h"
#include "scenes.h"
#include "ansi_colors.h"
#include "project_status.h"
#include "flag_system.h"
#include "time_utils.h"
#include "characters/mika.h"
#include "linenoise.h"
#include "character_data.h"
#include "items_data.h"
#include "event_system.h"
#include "ecc_time.h"
#include "render_utils.h"

// --- Global State ---
static uint32_t scene_entry_time = 0;

// --- Function Prototypes ---
void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index);
int is_numeric(const char* str);
static bool process_events(GameState* game_state, StoryScene* current_scene);

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
    g_argc = argc;
    g_argv = argv;
    init_mika_module();
    pthread_t time_thread_id;
    pthread_mutex_init(&time_mutex, NULL);
    init_event_queue();

    printf("Lain-day C version starting...\n");
    printf("Build Info - OS: %s, Arch: %s\n", BUILD_OS, BUILD_ARCH);

    GamePaths paths;
    init_paths(argv[0], &paths);

    if (!load_string_table()) {
        fprintf(stderr, "Error: Failed to load embedded string table.\n");
        return 1;
    }
    printf("String table loaded.\n");

    char session_name[MAX_NAME_LENGTH] = {0};
    char character_session_file_path[MAX_PATH_LENGTH] = {0};
    char session_dir_path[MAX_PATH_LENGTH] = {0};
    int arg_index = 1;
    g_arg_index_ptr = &arg_index;
    bool is_test_mode = false;

    GameState game_state;
    memset(&game_state, 0, sizeof(GameState));
    game_state.has_transient_message = false;
    g_game_state_ptr = &game_state;

    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-d") == 0) {
            is_test_mode = true;
            printf("Running in test mode (-d). Temporary session.\n");
        } else {
            fprintf(stderr, "Warning: Unknown argument '%s'. Ignoring.\n", argv[arg_index]);
        }
        arg_index++;
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
            printf("Using session name from argument: %s\n", session_name);
            arg_index++;
        } else {
             char* line = linenoise(get_string_by_id(TEXT_PROMPT_SESSION_NAME));
             if(line) {
                strncpy(session_name, line, MAX_NAME_LENGTH -1);
                session_name[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
                // linenoiseHistoryAdd(line); // Temporarily disable history to debug segfault
                free(line);
             } else {
                 fprintf(stderr, "Error: Failed to read session name, or EOF received.\n");
                 return 1; // Exit game
             }
        }
        if (strlen(session_name) == 0) { // Check if session_name is empty after input
            fprintf(stderr, "Error: Session name cannot be empty.\n");
            return 1;
        }

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

    if (!load_player_state(character_session_file_path, &game_state)) {
        fprintf(stderr, "Error: Failed to load player state from '%s'.\n", character_session_file_path);
        return 1;
    }
    printf("Player data loaded.\n");

    if (!load_map_data(paths.map_dir, &game_state)) return 1;
    printf("Map data loaded.\n");

    if (!load_items_data(&game_state)) return 1;
    printf("Items data loaded.\n");

    pthread_create(&time_thread_id, NULL, time_thread_func, (void*)&game_state);

    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;
    bool dirty = true;

    pthread_mutex_lock(&time_mutex);
    if (!transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
        fprintf(stderr, "Failed to initialize starting scene: %s\n", game_state.current_story_file);
        pthread_mutex_unlock(&time_mutex);
        return 1;
    }
    scene_entry_time = decode_time_with_ecc(game_state.time_of_day).data;
    pthread_mutex_unlock(&time_mutex);

    while (game_is_running) {
        get_next_input(input_buffer, sizeof(input_buffer), argc, argv, &arg_index);

        pthread_mutex_lock(&time_mutex);

        if (process_events(&game_state, &current_scene)) {
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
                    if (is_choice_selectable(&current_scene.choices[i], &game_state)) {
                        visible_choice_count++;
                        if (visible_choice_count == choice_num) {
                            target_array_index = i;
                            break;
                        }
                    }
                }
                if (target_array_index != -1) {
                    if (execute_action(current_scene.choices[target_array_index].action_id, &game_state)) {
                        dirty = true;
                    }
                } else {
                    printf("Invalid choice.\n");
                }
            } else {
                if (execute_command(input_buffer, &game_state)) {
                    dirty = true;
                }
            }
            if (game_is_running) dirty = true;
        }

        if (dirty) {
            if (strcmp(current_scene.scene_id, game_state.current_story_file) != 0) {
                if (transition_to_scene(game_state.current_story_file, &current_scene, &game_state)) {
                    scene_entry_time = decode_time_with_ecc(game_state.time_of_day).data;
                } else {
                    game_is_running = false;
                }
            }
            if (game_is_running) {
                render_current_scene(&current_scene, &game_state);
            }
            dirty = false;
        }

        pthread_mutex_unlock(&time_mutex);
        usleep(50000);
    }
    
    pthread_join(time_thread_id, NULL);
    
    if (!is_test_mode) {
        if (!save_game_state(character_session_file_path, &game_state)) {
             fprintf(stderr, "Error saving game state.\n");
        }
    }
    
    cleanup_game_state(&game_state);
    pthread_mutex_destroy(&time_mutex);

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
            char *line = linenoise(get_string_by_id(TEXT_PROMPT_INPUT_ARROW));
            if (line != NULL) {
                if (strlen(line) > 0) {
                    // linenoiseHistoryAdd(line); // Temporarily disable history to debug segfault
                }
                strncpy(buffer, line, buffer_size - 1);
                free(line);
            } else {
                strncpy(buffer, "quit", buffer_size - 1);
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