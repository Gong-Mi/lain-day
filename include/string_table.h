#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include "string_ids.h" // Includes StringID and TEXT_COUNT

// Function to initialize the string table, called by data_loader
void init_string_table(const char** strings, int count);

// Function to clean up the string table
void cleanup_string_table();

// Declare the public function to access strings by ID
const char* get_string_by_id(StringID id);

#endif // STRING_TABLE_H