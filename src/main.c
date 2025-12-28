#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <locale.h> 
#include <signal.h> 
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
#include "linenoise.h"
#include "string_id_names.h"
#include "systems/boot_system.h"

// --- Global State ---
static uint32_t scene_entry_time = 0;
volatile sig_atomic_t g_needs_redraw = 0;

void handle_signal(int sig) { (void)sig; g_needs_redraw = 1; }
void handle_sigwinch(int sig) { (void)sig; g_needs_redraw = 1; }

extern const char* g_embedded_strings[TEXT_COUNT];

// --- Function Prototypes ---
void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index);
int is_numeric(const char* str);
bool is_valid_session_name(const char* name);
int handle_key_event(int key, void* userdata);

static bool process_events(GameState* gs, StoryScene* scene) {
    return check_and_trigger_auto_events(gs, scene, scene_entry_time);
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    init_terminal_state();
    init_string_table(g_embedded_strings, TEXT_COUNT);
    
    g_argc = argc;
    g_argv = argv;
    init_mika_module();
    pthread_t time_thread_id;
    pthread_mutex_init(&time_mutex, NULL);
    init_event_queue();

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);

    printf("Lain-day C version starting...\n");

    game_state = malloc(sizeof(GameState));
    if (!game_state) return 1;
    memset(game_state, 0, sizeof(GameState));
    init_paths(argv[0], &game_state->paths);

    // --- 执行开机引导系统 ---
    char character_file_path[MAX_PATH_LENGTH] = {0};
    int arg_index = 1;
    if (!perform_boot_sequence(game_state, argc, argv, &arg_index, character_file_path)) {
        restore_terminal_state();
        return 0;
    }

    // --- 加载数据 ---
    if (!load_player_state(character_file_path, game_state)) {
        fprintf(stderr, "Fatal error loading state.\n");
        restore_terminal_state();
        return 1;
    }
    load_items_data(game_state);
    load_map_data(NULL, game_state);

    if (game_state->current_story_file[0] == '\0') {
        strncpy(game_state->current_story_file, "SCENE_00_ENTRY", MAX_PATH_LENGTH - 1);
    }

    // --- 进入游戏模式 ---
    enable_raw_mode();
    linenoiseSetKeyCallback(handle_key_event, game_state);
    if (is_mouse_supported()) linenoiseSetMouseSupport(2);

    pthread_create(&time_thread_id, NULL, time_thread_func, (void*)game_state);

    char input_buffer[MAX_LINE_LENGTH];
    StoryScene current_scene;
    bool dirty = true;
    bool game_is_running = true;

    while (game_is_running) {
        if (dirty) {
            pthread_mutex_lock(&time_mutex);
            if (game_state->current_story_file[0] != '\0') {
                transition_to_scene(game_state->current_story_file, &current_scene, game_state);
                game_state->current_story_file[0] = '\0';
            }
            render_current_scene(&current_scene, game_state);
            dirty = false;
            pthread_mutex_unlock(&time_mutex);
        }

        get_next_input(input_buffer, sizeof(input_buffer), argc, argv, &arg_index);
        
        pthread_mutex_lock(&time_mutex);
        if (input_buffer[0] != '\0') {
            if (strcmp(input_buffer, "quit") == 0) game_is_running = false;
            else if (is_numeric(input_buffer)) {
                int choice_num = atoi(input_buffer);
                int visible_choice_count = 0;
                for (int i = 0; i < current_scene.choice_count; i++) {
                    if (is_choice_selectable(&current_scene.choices[i], game_state)) {
                        if (++visible_choice_count == choice_num) {
                            if (execute_action(current_scene.choices[i].action_id, game_state)) dirty = true;
                            break;
                        }
                    }
                }
            } else {
                if (execute_command(input_buffer, game_state)) dirty = true;
            }
        }
        if (g_needs_redraw) { dirty = true; g_needs_redraw = 0; }
        if (process_events(game_state, &current_scene)) dirty = true;
        pthread_mutex_unlock(&time_mutex);
    }

    cleanup_game_state(game_state);
    restore_terminal_state();
    printf("\nLain-day C version exiting.\n");
    return 0;
}

void get_next_input(char* buffer, int buffer_size, int argc, char* argv[], int* arg_index) {
    if (argc > *arg_index) {
        strncpy(buffer, argv[(*arg_index)++], buffer_size - 1);
        return;
    }
    char prompt[128];
    snprintf(prompt, sizeof(prompt), "\x1b[1;32m%s@wired_navi\x1b[0m:\x1b[1;34m~\x1b[0m$ ", game_state->session_name);
    while (1) {
        char *line = linenoise(prompt);
        if (line == NULL) { strncpy(buffer, "quit", buffer_size - 1); return; }
        if (strlen(line) > 0) {
            int mx, my, mbtn, mevt;
            if (is_mouse_supported() && linenoiseGetLastMouse(&mx, &my, &mbtn, &mevt)) {
                if (mevt == 'M' && mbtn == 0) {
                    int click_offset = my - (game_state->choices_start_row + 1);
                    if (click_offset >= 0 && click_offset < (game_state->choice_row_count - 2)) {
                        snprintf(buffer, buffer_size, "%d", click_offset + 1);
                        free(line); return;
                    }
                }
                free(line); printf("\033[A\r\033[K"); fflush(stdout); continue;
            }
            strncpy(buffer, line, buffer_size - 1);
            free(line); return;
        }
        free(line);
    }
}

int is_numeric(const char* str) {
    if (!str || !*str) return 0;
    while (*str) { if (!isdigit((unsigned char)*str++)) return 0; }
    return 1;
}

int handle_key_event(int key, void* userdata) {
    GameState* gs = (GameState*)userdata;
    if (key == 3000) { // PageUp
        if (gs->scroll_offset > 0) { gs->scroll_offset--; g_needs_redraw = 1; }
        return 1;
    } else if (key == 3001) { // PageDown
        gs->scroll_offset++; g_needs_redraw = 1;
        return 1;
    }
    return 0;
}
