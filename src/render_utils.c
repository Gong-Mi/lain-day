#include "render_utils.h"
#include "scenes.h"
#include "ansi_colors.h"
#include "string_table.h" // Needed for get_string_by_id prototype
#include "ecc_time.h"
#include "characters/mika.h" // Needed for CharacterMika and get_mika_module
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For usleep

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
        // This part doesn't need typewriter effect
        printf("Location: %s\n", scene->location_id);
    }
    printf("========================================\n");

    // --- Check for character presence ---
    const CharacterMika* mika = get_mika_module();
    // Don't print this if the current scene is about Mika's room, as it would be redundant.
    if (strcmp(mika->current_location_id, game_state->player_state.location) == 0 &&
        strcmp(scene->scene_id, "SCENE_MIKA_ROOM_UNLOCKED") != 0) 
    {
        printf(ANSI_COLOR_YELLOW "\n你看到姐姐美香也在这里。\n" ANSI_COLOR_RESET);
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
