#include "render_utils.h"
#include "ansi_colors.h"
#include "string_table.h" // Needed for get_string_by_id
#include <stdio.h>
#include <string.h>
#include <unistd.h> // For usleep

void print_colored_line(SpeakerID speaker_id, StringID text_id, const struct GameState* game_state) {

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

        {SPEAKER_MIRA, "Mika", LAINS_SISTER_MIRA_COLOR},

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

void print_game_time(int time_of_day) {
    int hours = time_of_day / 60;
    int minutes = time_of_day % 60;
    printf(ANSI_COLOR_YELLOW "[%02d:%02d]" ANSI_COLOR_RESET "\n", hours, minutes);
}


void print_raw_text(const char* text) {
    if (text != NULL) {
        printf("%s\n", text);
    }
}
