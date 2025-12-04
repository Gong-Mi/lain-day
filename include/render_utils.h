#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "game_types.h" // For GameState definition

void print_game_time(uint32_t time_of_day);

// Prints a line of dialogue, handling speaker names and colors.
void print_colored_line(SpeakerID speaker_id, StringID text_id, const GameState* game_state);

// Prints a raw, unformatted line of text.
void print_raw_text(const char* text);

// Clears the terminal screen.
void clear_screen();

#endif // RENDER_UTILS_H
