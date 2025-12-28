#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <locale.h> 
#include <signal.h> 
#include <pthread.h>
#include <errno.h>
#include <sys/select.h> 

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
#include "logger.h"

static uint32_t scene_entry_time = 0;
volatile sig_atomic_t g_needs_redraw = 0;

extern volatile bool game_is_running;
extern pthread_mutex_t time_mutex;

void handle_signal(int sig) { (void)sig; g_needs_redraw = 1; }
void handle_sigwinch(int sig) { (void)sig; g_needs_redraw = 1; }

extern const char* g_embedded_strings[TEXT_COUNT];

int is_numeric(const char* str);
int handle_key_event(int key, void* userdata);

static bool process_events(GameState* gs, StoryScene* scene) {
    return check_and_trigger_auto_events(gs, scene, scene_entry_time);
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    logger_init("game_debug.log");
    init_terminal_state();
    init_string_table(g_embedded_strings, TEXT_COUNT);
    
    enter_fullscreen_mode();

    g_argc = argc;
    g_argv = argv;
    init_mika_module();
    pthread_mutex_init(&time_mutex, NULL);
    init_event_queue();

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);

    game_state = malloc(sizeof(GameState));
    if (!game_state) return 1;
    memset(game_state, 0, sizeof(GameState));
    init_paths(argv[0], &game_state->paths);

    char character_file_path[MAX_PATH_LENGTH] = {0};
    int arg_index = 1;
    if (!perform_boot_sequence(game_state, argc, argv, &arg_index, character_file_path)) {
        restore_terminal_state();
        return 0;
    }

    load_player_state(character_file_path, game_state);
    load_items_data(game_state);
    load_map_data(NULL, game_state);

    if (game_state->current_story_file[0] == '\0') {
        strncpy(game_state->current_story_file, "SCENE_00_ENTRY", MAX_PATH_LENGTH - 1);
    }

    enable_raw_mode();
    linenoiseSetKeyCallback(handle_key_event, game_state);
    
    // TEMPORARILY DISABLED MOUSE SUPPORT TO FIX INPUT ISSUE
    // if (is_mouse_supported()) linenoiseSetMouseSupport(2);
    
    // Removed timeout to allow proper blocking input in raw mode
    // linenoiseSetTimeout(10); 

    game_is_running = true; 
    pthread_t time_thread_id;
    pthread_create(&time_thread_id, NULL, time_thread_func, (void*)game_state);

    char prompt[128];
    snprintf(prompt, sizeof(prompt), "\x1b[1;32m%s@wired_navi\x1b[0m:\x1b[1;34m~\x1b[0m$ ", game_state->session_name);

    StoryScene current_scene;
    bool dirty = true;
    
    // Initial Render
    pthread_mutex_lock(&time_mutex);
    if (game_state->current_story_file[0] != '\0') {
        transition_to_scene(game_state->current_story_file, &current_scene, game_state);
        game_state->current_story_file[0] = '\0';
        scene_entry_time = decode_time_with_ecc(game_state->time_of_day).data;
    }
    render_current_scene(&current_scene, game_state);
    printf("%s", prompt); 
    fflush(stdout);
    pthread_mutex_unlock(&time_mutex);

    while (game_is_running) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 50000; // 50ms

        int retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        
        char *line = NULL;
        char input_buffer[MAX_LINE_LENGTH] = {0};
        bool input_handled = false;

        if (retval > 0) {
            // Clear prompt line before input
            printf("\r\033[K");
            fflush(stdout);
            
            line = linenoise(prompt);
            if (line) logger_log("Raw input from linenoise: '%s' (len: %lu)", line, (unsigned long)strlen(line));
            else logger_log("linenoise returned NULL");

            if (line == NULL && errno != EAGAIN) {
                game_is_running = false; 
                break;
            }
            
            /* 
            // MOUSE CHECK DISABLED
            int mx, my, mbtn, mevt;
            if (is_mouse_supported() && linenoiseGetLastMouse(&mx, &my, &mbtn, &mevt)) {
                // ... mouse logic ...
                if (line) { free(line); line = NULL; }
            } 
            else */ 
            
            if (line != NULL) {
                strncpy(input_buffer, line, sizeof(input_buffer)-1);
                free(line);
                input_handled = true;
            }

            // Prevent prompt sinking on empty input
            if (input_buffer[0] == '\0') {
                printf("\033[A");
                fflush(stdout);
            }
        }

        pthread_mutex_lock(&time_mutex);
        
        // Input processing
        if (input_handled && input_buffer[0] != '\0') {
            logger_log("Processing input_buffer: '%s'", input_buffer);
            if (strcmp(input_buffer, "quit") == 0) game_is_running = false;
            else if (is_numeric(input_buffer)) {
                logger_log("Input identified as numeric. Scene choice count: %d", current_scene.choice_count);
                int choice_num = atoi(input_buffer);
                int visible_choice_count = 0;
                for (int i = 0; i < current_scene.choice_count; i++) {
                    bool selectable = is_choice_selectable(&current_scene.choices[i], game_state);
                    logger_log("Checking Choice [%d]: selectable=%d, action='%s'", i, selectable, current_scene.choices[i].action_id);
                    if (selectable) {
                        if (++visible_choice_count == choice_num) {
                            logger_log("MATCH! Executing action for choice %d", i);
                            if (execute_action(current_scene.choices[i].action_id, game_state)) dirty = true;
                            break;
                        }
                    }
                }
            } else {
                logger_log("Input identified as command.");
                if (execute_command(input_buffer, game_state)) dirty = true;
            }
        }

        Event ev;
        bool time_ticked = false;
        while (poll_event(&ev)) {
            if (ev.type == TIME_TICK_EVENT) {
                time_ticked = true;
                if (process_events(game_state, &current_scene)) dirty = true;
            }
        }
        if (current_scene.is_takeover) dirty = true;
        if (g_needs_redraw) { dirty = true; g_needs_redraw = 0; }

        if (dirty) {
            printf("\r\033[K");
            if (game_state->current_story_file[0] != '\0') {
                transition_to_scene(game_state->current_story_file, &current_scene, game_state);
                game_state->current_story_file[0] = '\0';
                scene_entry_time = decode_time_with_ecc(game_state->time_of_day).data;
            }
            render_current_scene(&current_scene, game_state);
            
            if (current_scene.is_takeover) {
                update_time_display_inplace(game_state->time_of_day);
                printf("\r%s", prompt);
            } else {
                printf("\n%s", prompt);
            }
            fflush(stdout);
            dirty = false;
        } else if (time_ticked) {
            update_time_display_inplace(game_state->time_of_day);
        }

        pthread_mutex_unlock(&time_mutex);
    }

    restore_terminal_state();
    cleanup_game_state(game_state);
    logger_close();
    return 0;
}

int is_numeric(const char* str) {
    if (!str || !*str) return 0;
    while (isspace((unsigned char)*str)) str++; 
    if (!*str) return 0; 
    const char* end = str;
    while (*end) end++;
    end--;
    while (end > str && isspace((unsigned char)*end)) end--; 
    
    while (str <= end) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

int handle_key_event(int key, void* userdata) {
    GameState* gs = (GameState*)userdata;
    if (key == 3000) { if (gs->scroll_offset > 0) { gs->scroll_offset--; g_needs_redraw = 1; } return 1; }
    else if (key == 3001) { gs->scroll_offset++; g_needs_redraw = 1; return 1; }
    return 0;
}
