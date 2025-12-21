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

// --- Doll States ---
#define DOLL_STATE_NONE          0 // 不存在/未知
#define DOLL_STATE_NORMAL        1 // 普通
#define DOLL_STATE_DAMAGED       2 // 有伤
#define DOLL_STATE_CHIPPED       3 // 有芯片
#define DOLL_STATE_SPOKEN        4 // 说话了
#define DOLL_STATE_CORPSE        5 // 前周期尸体
#define DOLL_STATE_DAMAGED_CHIPPED 6 // 有伤且有芯片

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
    const char* view_scene_id;      // For 'arls': Scene to transition to for viewing contents (e.g., a fridge).
    const char* examine_action_id;  // For 'exper': Action to trigger on interaction (e.g., opening NAVI).
} POI;

#define MAX_CONDITIONS_PER_CHOICE 4

typedef struct {
    // Flag condition
    char flag_name[MAX_NAME_LENGTH];
    char required_value[MAX_NAME_LENGTH];

    // Time conditions (-1 means not used)
    int min_day;
    int max_day;
    int exact_day;
    int hour_start;
    int hour_end;
} Condition;

#define MAX_AUTO_EVENTS 4

typedef struct {
    char target_scene_id[MAX_NAME_LENGTH];
    int wait_time; // In seconds, 0 means instant
    char flag_to_set[MAX_NAME_LENGTH]; // Optional flag to set when triggered, value "1"
    Condition conditions[MAX_CONDITIONS_PER_CHOICE];
    int condition_count;
} AutoEvent;

typedef struct {
    char location_id[MAX_NAME_LENGTH];
} FrontMatter;

typedef enum {
    SPEAKER_NONE, SPEAKER_LAIN, SPEAKER_MOM, SPEAKER_DAD, SPEAKER_ALICE,
    SPEAKER_CHISA, SPEAKER_MIKA, SPEAKER_GHOST, SPEAKER_DOCTOR, SPEAKER_NAVI,
    SPEAKER_PARENT, SPEAKER_COUNT
} SpeakerID;

typedef struct {
    SpeakerID speaker_id;
    StringID text_id;
} DialogueLine;

typedef struct {
    StringID text_id;
    char action_id[MAX_NAME_LENGTH];
    Condition conditions[MAX_CONDITIONS_PER_CHOICE];
    int condition_count;
} StoryChoice;

typedef struct {
    char scene_id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char location_id[MAX_NAME_LENGTH];
    DialogueLine dialogue_lines[MAX_TEXT_LINES_PER_SCENE];
    int dialogue_line_count;
    StoryChoice choices[MAX_CHOICES_PER_SCENE];
    int choice_count;
    AutoEvent auto_events[MAX_AUTO_EVENTS];
    int auto_event_count;
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
    const char* target_scene_id; // Added: The scene to transition to upon successful connection
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
typedef struct GamePaths {
    char base_path[MAX_PATH_LENGTH];
    char items_file[MAX_PATH_LENGTH];
    char actions_file[MAX_PATH_LENGTH];
    char map_dir[MAX_PATH_LENGTH];
    char session_root_dir[MAX_PATH_LENGTH];
} GamePaths;

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
    int8_t doll_state_lain_room; // State of the doll in Lain's room
    int8_t doll_state_mika_room; // State of the doll in Mika's room
    char mika_location_storage[MAX_NAME_LENGTH]; // Storage for Mika's location string
    int mika_sanity_level; // Mika's current sanity level (0-3)
    GamePaths paths; // Add GamePaths struct here
} GameState;

// Global game state and configuration
extern struct GameState* game_state;
extern bool g_character_father_alive_compile_time; // Reflects CHARACTER_FATHER_ALIVE compile-time setting

// Globals for command-line arguments
extern int g_argc;
extern char** g_argv;
extern int* g_arg_index_ptr;


#endif // GAME_TYPES_H
