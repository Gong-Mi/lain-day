#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "cJSON.h" // Include cJSON.h for cJSON* type
#include "string_ids.h" // Include StringID enum

#define MAX_NAME_LENGTH 64
#define MAX_DESC_LENGTH 256
#define MAX_PATH_LENGTH 128
#define MAX_INVENTORY_ITEMS 32
#define MAX_POIS 16
#define MAX_CONNECTIONS 8
#define MAX_COMMANDS 32
#define MAX_FLAGS 8 // Max number of flags for an action

// Story parsing limits - now reference StringID for text content
#define MAX_TEXT_LINES_PER_SCENE 256
#define MAX_CHOICES_PER_SCENE 8
#define MAX_LINE_LENGTH 512 // Max length of a single text line or choice text

// A single item in the player's inventory (used in PlayerState)
typedef struct {
    char name[MAX_NAME_LENGTH];
    int quantity;
} InventoryItem;

// Represents the player's state
typedef struct {
    char location[MAX_NAME_LENGTH];
    int credit_level;
    InventoryItem inventory[MAX_INVENTORY_ITEMS];
    int inventory_count;
    char unlocked_commands[MAX_COMMANDS][MAX_NAME_LENGTH];
    int unlocked_commands_count;
} PlayerState;

// Represents an item in the game world (from items.json)
typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH];
    int required_credit;
} Item;

// Main Action struct (from actions.json)
// Stores payload as a raw cJSON object for flexibility
typedef struct {
    char id[MAX_NAME_LENGTH]; // e.g., "default_story_change_action"
    char type_str[MAX_NAME_LENGTH]; // Action type as string (e.g., "story_change")
    cJSON *payload_json; // Store the raw payload as a cJSON object, to be parsed at execution
} Action;

// Represents a single location in the game world
typedef struct {
    char id[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESC_LENGTH * 4]; // Longer for full descriptions
    char points_of_interest[MAX_POIS][MAX_DESC_LENGTH];
    int poi_count;
    char connections[MAX_CONNECTIONS][MAX_NAME_LENGTH]; // List of location IDs
    int connection_count;
} Location;

// Condition for a choice or scene element
typedef struct {
    char flag_name[MAX_NAME_LENGTH]; // Name of the flag to check
    int required_value;             // Value the flag must have
    // Add other condition types as needed (e.g., item in inventory, game time)
} ChoiceCondition;


// Front matter for a story scene (YAML-like block at top of MD file) - now primarily for context
typedef struct {
    char location_id[MAX_NAME_LENGTH]; // Location where this scene takes place
    // Add other front matter fields as needed (e.g., flags to set)
} FrontMatter;

// A single choice presented to the player
typedef struct {
    StringID text_id;                 // The choice text (using StringID)
    char action_id[MAX_NAME_LENGTH];  // The ID of the action to trigger (e.g., "talk_to_figure")
    ChoiceCondition condition;        // Condition for this choice to be visible/selectable
} StoryChoice;

// A single story scene loaded from a Markdown file
typedef struct {
    char scene_id[MAX_NAME_LENGTH]; // Unique ID for the scene
    // FrontMatter front_matter; // Can be integrated or kept for legacy if needed
    char location_id[MAX_NAME_LENGTH]; // Direct location ID for this scene
    StringID text_content_ids[MAX_TEXT_LINES_PER_SCENE]; // Array of StringIDs for text lines
    int text_line_count;
    StoryChoice choices[MAX_CHOICES_PER_SCENE];
    int choice_count;
    // Add other scene-specific data like character sprites, background art, etc.
} StoryScene;


// Represents the overall state of the game
#define MAX_LOCATIONS 64 // Assuming a max number of locations
#define MAX_ITEMS 64     // Assuming a max number of distinct items
#define MAX_ACTIONS 128  // Assuming a max number of distinct actions

typedef struct {
    PlayerState player_state;
    char current_story_file[MAX_PATH_LENGTH]; // Still used for legacy MD, or can be SceneID for C
    
    // Game data loaded at startup
    Location all_locations[MAX_LOCATIONS];
    int location_count;
    Item all_items[MAX_ITEMS];
    int item_count;
    Action all_actions[MAX_ACTIONS];
    int action_count;

    // Any global flags or state not tied to player
    // For now, keep it simple.
} GameState;

#endif // GAME_TYPES_H