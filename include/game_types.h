#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "string_ids.h" // For StringID
#include "flag_system.h" // For HashTable
#include "cJSON.h"
#include "cmap.h" // For CMap*
#include <pthread.h>

// --- Defines ---
#define MAX_NAME_LENGTH 64
#define MAX_DESC_LENGTH 256
#define MAX_PATH_LENGTH 128
#define MAX_INVENTORY_ITEMS 32
#define MAX_POIS 16
#define MAX_CONNECTIONS 8
#define MAX_COMMANDS 32
#define MAX_FLAGS 8
#define MAX_TEXT_LINES_PER_SCENE 256
#define MAX_CHOICES_PER_SCENE 8
#define MAX_LINE_LENGTH 512
#define MAX_LOCATIONS 64
#define MAX_ITEMS 64
#define MAX_ACTIONS 128

// --- Forward Declarations for Circular Dependencies ---
struct GameState;
struct Connection;
struct Location_struct;

// --- Primitive Structs (no major dependencies) ---

typedef struct {
    char name[MAX_NAME_LENGTH];
    int quantity;
} InventoryItem;

typedef struct {
    char location[MAX_NAME_LENGTH];
    int credit_level;
    InventoryItem inventory[MAX_INVENTORY_ITEMS];
    int inventory_count;
    char unlocked_commands[MAX_COMMANDS][MAX_NAME_LENGTH];
    int unlocked_commands_count;
} PlayerState;

typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH];
    int required_credit;
} Item;

typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH * 2];
    const char* examine_scene_id; // Scene to transition to when examined
} POI;

typedef struct {
    char flag_name[MAX_NAME_LENGTH];
    int required_value;
} ChoiceCondition;

typedef struct {
    char location_id[MAX_NAME_LENGTH];
} FrontMatter;

typedef enum {
    SPEAKER_NONE, SPEAKER_LAIN, SPEAKER_MOM, SPEAKER_DAD, SPEAKER_ALICE,
    SPEAKER_CHISA, SPEAKER_MIRA, SPEAKER_GHOST, SPEAKER_DOCTOR, SPEAKER_NAVI,
    SPEAKER_PARENT, SPEAKER_COUNT
} SpeakerID;

typedef struct {
    SpeakerID speaker_id;
    StringID text_id;
} DialogueLine;

typedef struct {
    StringID text_id;
    char action_id[MAX_NAME_LENGTH];
    ChoiceCondition condition;
} StoryChoice;

typedef struct {
    char scene_id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char location_id[MAX_NAME_LENGTH];
    DialogueLine dialogue_lines[MAX_TEXT_LINES_PER_SCENE];
    int dialogue_line_count;
    StoryChoice choices[MAX_CHOICES_PER_SCENE];
    int choice_count;
} StoryScene;


// --- Main Structs with Interdependencies ---

// Typedef for the accessibility check function pointer
// Uses forward-declared types, which is why we use 'struct GameState'
typedef bool (*is_accessible_func)(struct GameState*, const struct Connection*);

// Connection Struct
typedef struct Connection {
    const char* target_location_id;
    const char* action_id;
    is_accessible_func is_accessible;
    const char* access_denied_scene_id;
} Connection;

// Location Struct (depends on POI and Connection)
typedef struct Location_struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH * 4];
    POI pois[MAX_POIS];
    int pois_count;
    Connection connections[MAX_CONNECTIONS];
    int connection_count;
} Location;

// GameState Struct (depends on PlayerState, Location, Item)
typedef struct GameState {
    PlayerState player_state;
    char current_story_file[MAX_PATH_LENGTH];
    uint32_t time_of_day;
    Location all_locations[MAX_LOCATIONS];
    int location_count;
    CMap* location_map;
    Item all_items[MAX_ITEMS];
    int item_count;
    HashTable* flags;
    float typewriter_delay;
    int navi_progress_style;
    char transient_message[MAX_LINE_LENGTH];
    bool has_transient_message;
} GameState;

// Global pointer to the GameState instance
extern GameState* g_game_state_ptr;

#endif // GAME_TYPES_H
