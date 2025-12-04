#include "conditions.h"
#include "game_types.h" // For the full GameState definition
#include "time_utils.h" // For time_mutex and game_is_running
#include "ecc_time.h"   // For ECC time decoding
#include <pthread.h>

extern pthread_mutex_t time_mutex;
