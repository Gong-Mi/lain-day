#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include "string_ids.h" // For StringID enum

// Declare the global string table
extern const char* g_string_table[];

// Function to retrieve a string by its ID
const char* get_string_by_id(StringID id);

#endif // STRING_TABLE_H
