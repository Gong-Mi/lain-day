#include "../include/string_ids.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For strdup

// Global pointer to store the loaded strings
const char* g_string_table[TEXT_COUNT];

// Function to initialize the string table
// This function will be called by the data loader
void init_string_table(const char** strings, int count) {
    // Check if table is already initialized (first element is not NULL)
    if (g_string_table[0] != NULL) {
        // Free existing data if re-initializing
        for (int i = 0; i < TEXT_COUNT; ++i) { // Use TEXT_COUNT as the fixed size
            if (g_string_table[i] != NULL) {
                free((void*)g_string_table[i]);
                g_string_table[i] = NULL;
            }
        }
    }

    // Assign new strings
    for (int i = 0; i < count; ++i) {
        if (i < TEXT_COUNT) { // Ensure we don't go out of bounds of g_string_table
            g_string_table[i] = strdup(strings[i]); // Duplicate string to own memory
        }
    }
}

// Function to clean up the string table
void cleanup_string_table() {
    if (g_string_table[0] != NULL) { // Check if table was initialized
        for (int i = 0; i < TEXT_COUNT; ++i) { // Use TEXT_COUNT as the fixed size
            if (g_string_table[i] != NULL) {
                free((void*)g_string_table[i]);
                g_string_table[i] = NULL;
            }
        }
    }
}


const char* get_string_by_id(StringID id) {
    if (g_string_table[0] == NULL || id < 0 || id >= TEXT_COUNT) {
        // Fallback for uninitialized table or invalid ID
        // For TEXT_INVALID, we can hardcode a fallback if table is not loaded yet
        if (id == TEXT_INVALID) {
            return "ERROR: String table not loaded / Invalid Text ID";
        }
        return "ERROR: String not found (ID out of bounds or table not loaded)";
    }
    return g_string_table[id];
}
