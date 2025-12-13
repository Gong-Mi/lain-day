#ifndef NAVI_MINI_H
#define NAVI_MINI_H

// This system is for Lain's desktop computer (NAVI mini) interface.

#include "game_types.h"

/**
 * @brief Enter the NAVI mini (desktop) interface.
 * 
 * This function takes control of the game loop to display the 
 * NAVI mini system interface. It's designed to represent the
 * content from 01a_examine_navi.md.
 * 
 * @param game_state Pointer to the global game state.
 */
void enter_navi_mini(GameState* game_state);

#endif // NAVI_MINI_H
