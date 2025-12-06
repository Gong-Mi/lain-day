#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "game_types.h" // For GameState definition
#include "string_ids.h" // For StringID
#include "characters/mika.h" // For CharacterMika and get_mika_module

// Function prototypes
void print_game_time(uint32_t time_of_day);
void print_colored_line(SpeakerID speaker_id, StringID text_id, const GameState* game_state);
void print_raw_text(const char* text);
void clear_screen();

// Main scene rendering function
void render_current_scene(const StoryScene* scene, const struct GameState* game_state);

#endif // RENDER_UTILS_H
