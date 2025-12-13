#include "game_types.h"
#include <stdbool.h> // For bool type

// Define the global GameState pointer
GameState* game_state = NULL;

// Global variable reflecting the compile-time setting of CHARACTER_FATHER_ALIVE
#ifdef CHARACTER_FATHER_ALIVE
bool g_character_father_alive_compile_time = true;
#else
bool g_character_father_alive_compile_time = false;
#endif

// Define global variables for command-line arguments
int g_argc = 0;
char** g_argv = NULL;
int* g_arg_index_ptr = NULL;

