#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "game_types.h" // For GameState definition

// Prints a line of text, applying color coding for speakers.
void print_colored_line(const char* line, const struct GameState* game_state);

// Clears the terminal screen.
void clear_screen();

#endif // RENDER_UTILS_H
