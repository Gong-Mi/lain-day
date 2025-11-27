#ifndef FLAG_SYSTEM_H
#define FLAG_SYSTEM_H

// Node for handling collisions in the hash table (using chaining)
typedef struct FlagNode {
    char* key;
    char* value;
    struct FlagNode* next;
} FlagNode;

// The hash table structure
typedef struct HashTable {
    FlagNode** buckets; // Array of pointers to FlagNodes
    int size;         // Size of the buckets array
} HashTable;

/**
 * @brief Creates a new hash table.
 * 
 * @param size The number of buckets in the hash table.
 * @return A pointer to the newly created HashTable, or NULL on failure.
 */
HashTable* create_hash_table(int size);

/**
 * @brief Frees all memory associated with a hash table.
 * 
 * @param table The hash table to free.
 */
void free_hash_table(HashTable* table);

/**
 * @brief Sets a key-value pair in the hash table.
 *        If the key already exists, its value is updated.
 *        The table takes ownership of copies of the key and value.
 * 
 * @param table The hash table.
 * @param key The key to set.
 * @param value The value to associate with the key.
 */
void hash_table_set(HashTable* table, const char* key, const char* value);

/**
 * @brief Gets the value associated with a key from the hash table.
 * 
 * @param table The hash table.
 * @param key The key to look up.
 * @return The value as a constant char pointer, or NULL if the key is not found.
 */
const char* hash_table_get(HashTable* table, const char* key);

#endif // FLAG_SYSTEM_H
