#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include "string_ids.h" // Includes StringID and TEXT_COUNT

// Declare the global string table (defined in string_table.c)
extern const char* g_string_table[];

// Declare the public function to access strings by ID
const char* get_string_by_id(StringID id);

#endif // STRING_TABLE_H