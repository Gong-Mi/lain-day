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
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    int term_width = w.ws_col;
    int term_height = w.ws_row;

    // Adjust for font aspect ratio (approximately 2:1 height:width for typical characters)
    // We want to fit the image into term_width x (term_height - padding)
    // Let's reserve some lines for prompt
    int max_display_height = term_height - 6; 
    if (max_display_height < 10) max_display_height = 10;

    // Calculate scaling factors
    // We want to output '  ' (2 spaces) per pixel to approximate a square pixel.
    // So visual width is 2 * width.
    
    // Target dimensions
    int target_width = term_width / 2; // We use 2 chars per pixel
    int target_height = max_display_height;

    // Calculate scale to fit
    float scale_w = (float)width / target_width;
    float scale_h = (float)height / target_height;
    
    // Use the larger scale to fit both dimensions (fit inside)
    float scale = (scale_w > scale_h) ? scale_w : scale_h;
    
    // If image is smaller than target, we might not want to upscale too much, 
    // but for "adaptive" usually we want it large. Let's stick to fit.
    if (scale < 1.0f) scale = 1.0f; // Don't upscale, just center? Or allow upscale?
    // Let's allow downscaling only for now to ensure quality, or simple nearest neighbor.
    
    int render_width = width / scale;
    int render_height = height / scale;

    // Simple nearest neighbor sampling
    for (int y = 0; y < render_height; y++) {
        for (int x = 0; x < render_width; x++) {
            // Map render coordinate to source coordinate
            int src_x = (int)(x * scale);
            int src_y = (int)(y * scale);
            
            if (src_x >= width) src_x = width - 1;
            if (src_y >= height) src_y = height - 1;

            int index = (src_y * width + src_x) * 3;
            uint8_t r = data[index];
            uint8_t g = data[index+1];
            uint8_t b = data[index+2];

            // Print ANSI RGB background code with 2 spaces
            printf("\x1b[48;2;%d;%d;%dm  ", r, g, b);
        }
        printf("\x1b[0m\n"); // Reset color at end of row
    }
    printf("\x1b[0m"); // Ensure reset
    
    ImageBounds bounds;
    bounds.start_x = 1;
    bounds.start_y = 1;
    bounds.end_x = 1 + (render_width * 2);
    bounds.end_y = 1 + render_height;
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

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\x1b[?1000h\x1b[?1006h"); // Enable mouse tracking
    fflush(stdout);
}

void disable_raw_mode() {
    printf("\x1b[?1000l\x1b[?1006l"); // Disable mouse tracking
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void flush_input_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

void restore_terminal_state() {
    disable_raw_mode();
    exit_fullscreen_mode();
}
