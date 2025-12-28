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

void render_scene_description(const char* description); // Added
void render_text(const char* text); // Added
typedef struct {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
} ImageBounds;

void render_poi_name(const char* name); // Added
ImageBounds render_image_adaptively(const uint8_t* data, int width, int height); // Updated return type

void enter_fullscreen_mode();

void exit_fullscreen_mode();

void init_terminal_state(); // Added

bool is_mouse_supported();
void enable_raw_mode();
void disable_raw_mode();
void set_terminal_echo(bool enabled);
void flush_input_buffer();
void restore_terminal_state();

// Main scene rendering function
void render_current_scene(const StoryScene* scene, const struct GameState* game_state);

#endif // RENDER_UTILS_H
