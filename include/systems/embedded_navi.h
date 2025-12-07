#ifndef EMBEDDED_NAVI_H
#define EMBEDDED_NAVI_H

#include "game_types.h"

/**
 * @brief Enter the Embedded NAVI interface.
 * 
 * This function takes control of the game loop to display the 
 * NAVI system interface (CLI style). It handles input and 
 * state management for the NAVI subsystem until the user exits.
 * 
 * @param game_state Pointer to the global game state.
 */
void enter_embedded_navi(GameState* game_state);

#endif // EMBEDDED_NAVI_H
