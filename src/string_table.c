#include "../include/string_ids.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For strdup

// Global pointer to store the loaded strings
static const char** g_string_table_data = NULL;
static int g_string_table_count = 0;

// Function to initialize the string table
// This function will be called by the data loader
void init_string_table(const char** strings, int count) {
    if (g_string_table_data != NULL) {
        // Free existing data if re-initializing
        for (int i = 0; i < g_string_table_count; ++i) {
            free((void*)g_string_table_data[i]);
        }
        free(g_string_table_data);
    }

    g_string_table_count = count;
    g_string_table_data = (const char**)malloc(sizeof(const char*) * count);
    if (g_string_table_data == NULL) {
        // Handle allocation error
        g_string_table_count = 0;
        return;
    }

    for (int i = 0; i < count; ++i) {
        g_string_table_data[i] = strdup(strings[i]); // Duplicate string to own memory
    }
}

// Function to clean up the string table
void cleanup_string_table() {
    if (g_string_table_data != NULL) {
        for (int i = 0; i < g_string_table_count; ++i) {
            free((void*)g_string_table_data[i]);
        }
        free(g_string_table_data);
        g_string_table_data = NULL;
        g_string_table_count = 0;
    }
}


const char* get_string_by_id(StringID id) {
    if (g_string_table_data == NULL || id < 0 || id >= g_string_table_count) {
        // Fallback for uninitialized table or invalid ID
        // For TEXT_INVALID, we can hardcode a fallback if table is not loaded yet
        if (id == TEXT_INVALID) {
            return "ERROR: String table not loaded / Invalid Text ID";
        }
        return "ERROR: String not found (ID out of bounds or table not loaded)";
    }
    return g_string_table_data[id];
}
