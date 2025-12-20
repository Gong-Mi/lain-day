#include "render_utils.h"
#include "scenes.h"
#include "ansi_colors.h"
#include "string_table.h" // Needed for get_string_by_id prototype
#include "ecc_time.h"
#include "characters/mika.h" // Needed for CharacterMika and get_mika_module
#include "map_loader.h" // Needed for get_location_by_id
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For usleep
#include <sys/ioctl.h> // For ioctl
#include <stdint.h>  // For uint8_t
#include <termios.h> // For raw mode

// Helper function prototype
static void _render_transient_message(GameState* game_state);

void print_colored_line(SpeakerID speaker_id, StringID text_id, const GameState* game_state) {

    const char* line_text = get_string_by_id(text_id);

    if (line_text == NULL) {
        printf("\n");
        fflush(stdout);
        return;
    }

    // Map SpeakerID to name and color
    struct {
        SpeakerID id;
        const char* name;
        const char* color_code;
    } speaker_info[] = {
        {SPEAKER_LAIN, "你", ANSI_COLOR_CYAN},
        {SPEAKER_MOM, "妈妈", ANSI_COLOR_MAGENTA},
        {SPEAKER_DAD, "爸爸", FATHER_COLOR},
        {SPEAKER_ALICE, "Alice", ALICE_COLOR},
        {SPEAKER_CHISA, "Chisa", CHISA_COLOR},
        {SPEAKER_MIKA, "Mika", LAINS_SISTER_MIRA_COLOR},
        {SPEAKER_GHOST, "幽灵", ANSI_COLOR_RED},
        {SPEAKER_DOCTOR, "米良柊子", FUYUKO_MIRA_COLOR},
        {SPEAKER_NAVI, "Navi", NAVI_COLOR},
        {SPEAKER_PARENT, "父母", ANSI_COLOR_YELLOW},
        {SPEAKER_NONE, "", ANSI_COLOR_RESET}
    };

    const char* speaker_name = "";
    const char* speaker_color = ANSI_COLOR_RESET;

    for (int i = 0; i < SPEAKER_COUNT; i++) {
        if (speaker_info[i].id == speaker_id) {
            speaker_name = speaker_info[i].name;
            speaker_color = speaker_info[i].color_code;
            break;
        }
    }

    // Print directly to avoid large stack buffer
    char speaker_prefix[MAX_NAME_LENGTH + 10]; // Buffer for speaker name and color codes
    if (speaker_id != SPEAKER_NONE) {
        snprintf(speaker_prefix, sizeof(speaker_prefix), "%s%s: %s", speaker_color, speaker_name, ANSI_COLOR_RESET);
    } else {
        speaker_prefix[0] = '\0'; // No prefix for SPEAKER_NONE
    }

#ifdef USE_TYPEWRITER_EFFECT
    // Print speaker prefix at once
    if (speaker_prefix[0] != '\0') {
        printf("%s", speaker_prefix);
    }
    // Print dialogue line char by char
    for (int i = 0; line_text[i] != '\0'; i++) {
        putchar(line_text[i]);
        fflush(stdout);
        usleep((useconds_t)(game_state->typewriter_delay * 1000000));
    }
    printf("\n");
#else
    // Standard instant print
    if (speaker_prefix[0] != '\0') {
        printf("%s%s\n", speaker_prefix, line_text);
    } else {
        printf("%s\n", line_text);
    }
#endif
    fflush(stdout);
}

void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void print_game_time(uint32_t time_of_day) {
    DecodedTimeResult decoded_result = decode_time_with_ecc(time_of_day);
    
    // Handle uncorrectable errors by showing a glitchy time
    if (decoded_result.status == DOUBLE_BIT_ERROR_DETECTED) {
        printf(ANSI_COLOR_RED "[##:##]" ANSI_COLOR_RESET "\n");
        return;
    }

    uint32_t raw_units = decoded_result.data;
    uint32_t total_seconds = raw_units / 16;
    uint32_t total_minutes = total_seconds / 60;
    
    int hours = (total_minutes / 60) % 24;
    int minutes = total_minutes % 60;

    printf(ANSI_COLOR_YELLOW "[%02d:%02d]" ANSI_COLOR_RESET "\n", hours, minutes);
}

void print_raw_text(const char* text) {
    if (text != NULL) {
        printf("%s\n", text);
    }
}

// Function to render the description of a scene/location
void render_scene_description(const char* description) {
    if (description != NULL && strlen(description) > 0) {
        printf("\n%s\n", description);
    }
}

// Function to render generic text (similar to print_raw_text but distinct if needed later)
void render_text(const char* text) {
    if (text != NULL && strlen(text) > 0) {
        printf("%s", text); // No newline here, allows caller to control
    }
}

// Function to render a POI name
void render_poi_name(const char* name) {
    if (name != NULL && strlen(name) > 0) {
        printf(ANSI_COLOR_BLUE "  - %s" ANSI_COLOR_RESET, name);
    }
}

// Helper function to render and clear transient messages
static void _render_transient_message(GameState* game_state) {
    if (game_state->has_transient_message) {
        printf(ANSI_COLOR_BRIGHT_BLACK "\n%s\n" ANSI_COLOR_RESET, game_state->transient_message);
        memset(game_state->transient_message, 0, MAX_LINE_LENGTH); // Clear message content
        game_state->has_transient_message = false; // Reset flag
    }
}

// Function to render the current scene
void render_current_scene(const StoryScene* scene, const struct GameState* game_state) {
    #ifdef USE_CLEAR_SCREEN
    clear_screen(); // Clear screen for each scene render
    #endif
    print_game_time(game_state->time_of_day); // Print time at the top-left
    #ifdef USE_DEBUG_LOGGING
    fprintf(stderr, "DEBUG: Entering render_current_scene.\n");
    fprintf(stderr, "DEBUG: render_current_scene: scene ptr: %p\n", (void*)scene);
    fprintf(stderr, "DEBUG: Rendering scene: %s (ID: %s)\n", scene->name, scene->scene_id);
    #endif
    if (scene == NULL) {
        printf("Error: Scene is NULL.\n");
        return;
    }

    printf("\n========================================\n");
    if (scene->location_id[0] != '\0') {
        // Look up the localized location name
        Location* loc = get_location_by_id(scene->location_id);
        if (loc && loc->name[0] != '\0') {
            printf("Location: %s\n", loc->name);
        } else {
            printf("Location: %s\n", scene->location_id);
        }
    }
    printf("========================================\n");

    // --- Check for character presence ---
    const CharacterMika* mika = get_mika_module();
    // Don't print this if the current scene is about Mika's room, as it would be redundant.
    if (strcmp(mika->current_location_id, game_state->player_state.location) == 0 &&
        strcmp(scene->scene_id, "SCENE_MIKA_ROOM_UNLOCKED") != 0) 
    {
        printf(ANSI_COLOR_YELLOW "你看到姐姐美香也在这里。\n" ANSI_COLOR_RESET);
    }
    // --- End check for character presence ---

    for (int i = 0; i < scene->dialogue_line_count; i++) {
    #ifdef USE_STRING_DEBUG_LOGGING
        fprintf(stderr, "DEBUG:   Printing DialogueLine: speaker=%d, text_id=%d (%s)\n",
                scene->dialogue_lines[i].speaker_id, scene->dialogue_lines[i].text_id,
                get_string_by_id(scene->dialogue_lines[i].text_id));
    #endif
        print_colored_line(scene->dialogue_lines[i].speaker_id, scene->dialogue_lines[i].text_id, (GameState*)game_state); // Cast to GameState*
    }

    if (scene->choice_count > 0) {
        printf("\n--- Choices ---\n");
        int visible_choice_index = 1;
        for (int i = 0; i < scene->choice_count; i++) {
            const StoryChoice* choice = &scene->choices[i];
            
            if (is_choice_selectable(choice, game_state)) {
                printf("%d. %s\n", visible_choice_index++, get_string_by_id(choice->text_id));
            } else {
                // Print disabled choice in gray and without a number
                printf("   %s%s%s\n", ANSI_COLOR_BRIGHT_BLACK, get_string_by_id(choice->text_id), ANSI_COLOR_RESET);
            }
        }
        printf("---------------\n");
    }
    _render_transient_message((GameState*)game_state);
}

// Function to render an image adaptively to the terminal size
ImageBounds render_image_adaptively(const uint8_t* data, int width, int height) {
    struct winsize w;
    int term_width = 80;
    int term_height = 24;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        term_width = w.ws_col;
        term_height = w.ws_row;
    }

    // Safety margins to avoid auto-wrap and leave room for prompt
    int avail_w = term_width - 2;
    int avail_h = term_height - 4;

    if (avail_w < 2) avail_w = 2;
    if (avail_h < 2) avail_h = 2;

    // We use 2 spaces per pixel, so target pixel width is avail_w / 2
    float scale_w = (float)width / (avail_w / 2);
    float scale_h = (float)height / avail_h;
    
    // Use larger scale to fit inside
    float scale = (scale_w > scale_h) ? scale_w : scale_h;
    if (scale < 1.0f) scale = 1.0f; // Limit to 1:1, or upscale? Let's stay 1:1 max for now.
    
    int render_w = (int)(width / scale);
    int render_h = (int)(height / scale);

    // Centering offsets
    int ox = (term_width - (render_w * 2)) / 2;
    int oy = (term_height - 2 - render_h) / 2; // -2 for prompt room
    if (oy < 0) oy = 0;
    if (ox < 0) ox = 0;

    // Vertical padding
    for (int i = 0; i < oy; i++) printf("\n");

    for (int y = 0; y < render_h; y++) {
        // Horizontal padding
        for (int i = 0; i < ox; i++) printf(" ");
        
        for (int x = 0; x < render_w; x++) {
            int sx = (int)(x * scale);
            int sy = (int)(y * scale);
            
            if (sx >= width) sx = width - 1;
            if (sy >= height) sy = height - 1;

            int index = (sy * width + sx) * 3;
            uint8_t r = data[index];
            uint8_t g = data[index+1];
            uint8_t b = data[index+2];

            printf("\x1b[48;2;%d;%d;%dm  ", r, g, b);
        }
        printf("\x1b[0m\n"); // Reset color and new line
    }
    printf("\x1b[0m\n");
    fflush(stdout);
    
    ImageBounds bounds;
    bounds.start_x = ox + 1;
    bounds.start_y = oy + 1;
    bounds.end_x = ox + (render_w * 2);
    bounds.end_y = oy + render_h;
    return bounds;
}

void enter_fullscreen_mode() {
    // Switch to alternate screen buffer
    printf("\033[?1049h");
    // Move cursor to home
    printf("\033[H");
    // Clear screen
    printf("\033[2J");
    fflush(stdout);
}

void exit_fullscreen_mode() {
    // Switch back to normal screen buffer
    printf("\033[?1049l");
    // Show cursor (in case it was hidden)
    printf("\033[?25h");
    fflush(stdout);
}

static struct termios orig_termios;
static bool terminal_state_captured = false;

void init_terminal_state() {
    if (!terminal_state_captured) {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
            terminal_state_captured = true;
            // Ensure our "original" state has sane output flags just in case
            // we captured it while the terminal was already messy.
            orig_termios.c_oflag |= (OPOST | ONLCR);
            orig_termios.c_lflag |= (ECHO | ICANON | ISIG);
        }
    }
}

void enable_raw_mode() {
    init_terminal_state(); // Ensure we have a sane base
    struct termios raw = orig_termios;
    // Minimal raw mode: just disable echo and line buffering
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\x1b[?1000h\x1b[?1006h"); // Enable mouse tracking
    fflush(stdout);
}

void disable_raw_mode() {
    if (terminal_state_captured) {
        printf("\x1b[?1000l\x1b[?1006l"); // Disable mouse tracking
        fflush(stdout);
        
        // Restore to the sane original state
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

void set_terminal_echo(bool enabled) {
    if (!terminal_state_captured) init_terminal_state();
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &t) == 0) {
        if (enabled) t.c_lflag |= ECHO;
        else t.c_lflag &= ~ECHO;
        // Ensure output processing remains ON during intro
        t.c_oflag |= (OPOST | ONLCR);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
}

void flush_input_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH);
    // ... rest of robust flush stays same ...

    // Robustly drain any remaining input using select/read
    fd_set readfds;
    struct timeval tv;
    char buffer[256];
    int n;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 10000; // 10ms timeout

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
            n = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (n <= 0) break;
        } else {
            break;
        }
    }
}

void restore_terminal_state() {
    disable_raw_mode();
    exit_fullscreen_mode();
    // Force reset attributes, show cursor, and return to column 0
    printf("\x1b[0m\x1b[?25h\r"); 
    fflush(stdout);
}