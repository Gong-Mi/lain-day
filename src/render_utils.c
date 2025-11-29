#include "render_utils.h"
#include "ansi_colors.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For usleep

void print_colored_line(const char* line, const struct GameState* game_state) {
    if (line == NULL) {
        printf("\n");
        fflush(stdout);
        return;
    }

    // List of speakers and their colors.
    struct {
        const char* name_prefix;
        const char* color_code;
    } speakers[] = {
        {"Alice:", ALICE_COLOR},
        {"Chisa:", CHISA_COLOR},
        {"Father:", FATHER_COLOR},
        {"米良柊子:", FUYUKO_MIRA_COLOR}, // Doctor
        {"Mira:", LAINS_SISTER_MIRA_COLOR}, // Lain's Sister
        {"幽灵:", ANSI_COLOR_RED}, // Ghost
        {"爸爸:", FATHER_COLOR}, // Father
        {"妈妈:", ANSI_COLOR_MAGENTA}, // Mom
    };
    int num_speakers = sizeof(speakers) / sizeof(speakers[0]);

#ifdef USE_TYPEWRITER_EFFECT
    // --- Typewriter Effect Implementation ---
    const char* content_start = line;
    for (int i = 0; i < num_speakers; i++) {
        size_t prefix_len = strlen(speakers[i].name_prefix);
        if (strncmp(line, speakers[i].name_prefix, prefix_len) == 0) {
            // Found a speaker prefix, print it at once
            printf("%s%s%s", speakers[i].color_code, speakers[i].name_prefix, ANSI_COLOR_RESET);
            content_start += prefix_len;
            break;
        }
    }

    // Print the rest of the line char by char
    for (int i = 0; content_start[i] != '\0'; i++) {
        putchar(content_start[i]);
        fflush(stdout);
        usleep((useconds_t)(game_state->typewriter_delay * 1000000));
    }

    printf("\n");
    fflush(stdout);

#else
    // --- Standard Instant Implementation ---
    for (int i = 0; i < num_speakers; i++) {
        size_t prefix_len = strlen(speakers[i].name_prefix);
        if (strncmp(line, speakers[i].name_prefix, prefix_len) == 0) {
            // Found a speaker prefix
            printf("%s%s%s%s\n", speakers[i].color_code, speakers[i].name_prefix, ANSI_COLOR_RESET, line + prefix_len);
            return;
        }
    }

    // No speaker detected, print the line as is
    printf("%s\n", line);
#endif
}

void clear_screen() {
    printf("\033[2J\033[H");
}
