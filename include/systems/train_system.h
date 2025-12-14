#ifndef TRAIN_SYSTEM_H
#define TRAIN_SYSTEM_H

#include "game_types.h"

/**
 * @brief Enters the ticket machine interface.
 * 
 * This function takes over the game loop to present the user with a special
 * interface for interacting with the train system, stylized as a ticket machine.
 * 
 * @param game_state A pointer to the current game state.
 */
void enter_ticket_machine_interface(GameState* game_state);

#endif // TRAIN_SYSTEM_H
