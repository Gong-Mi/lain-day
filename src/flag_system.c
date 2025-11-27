#include "flag_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A simple and widely used hash function (djb2)
static unsigned int hash_function(const char* key, int table_size) {
    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % table_size;
}

// Helper to create a new flag node
static FlagNode* create_flag_node(const char* key, const char* value) {
    FlagNode* new_node = (FlagNode*)malloc(sizeof(FlagNode));
    if (!new_node) {
        return NULL;
    }

    new_node->key = strdup(key);
    new_node->value = strdup(value);
    new_node->next = NULL;

    if (!new_node->key || !new_node->value) {
        // Handle strdup allocation failure
        free(new_node->key);
        free(new_node->value);
        free(new_node);
        return NULL;
    }

    return new_node;
}

HashTable* create_hash_table(int size) {
    if (size <= 0) {
        return NULL;
    }

    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    if (!table) {
        return NULL;
    }

    table->size = size;
    table->buckets = (FlagNode**)calloc(table->size, sizeof(FlagNode*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }

    return table;
}

void free_hash_table(HashTable* table) {
    if (!table) {
        return;
    }

    for (int i = 0; i < table->size; i++) {
        FlagNode* current = table->buckets[i];
        while (current) {
            FlagNode* to_free = current;
            current = current->next;
            free(to_free->key);
            free(to_free->value);
            free(to_free);
        }
    }

    free(table->buckets);
    free(table);
}

void hash_table_set(HashTable* table, const char* key, const char* value) {
    if (!table || !key || !value) {
        return;
    }

    unsigned int index = hash_function(key, table->size);

    FlagNode* current = table->buckets[index];
    FlagNode* prev = NULL;

    // Traverse the linked list (if any) to find the key
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Key found, update the value
            free(current->value);
            current->value = strdup(value);
            if (!current->value) {
                // Handle allocation failure, maybe log an error
            }
            return;
        }
        prev = current;
        current = current->next;
    }

    // Key not found, create a new node and add it
    FlagNode* new_node = create_flag_node(key, value);
    if (!new_node) {
        // Handle allocation failure
        return;
    }

    if (prev) {
        // Add to the end of the list
        prev->next = new_node;
    } else {
        // List was empty, this is the first node
        table->buckets[index] = new_node;
    }
}

const char* hash_table_get(HashTable* table, const char* key) {
    if (!table || !key) {
        return NULL;
    }

    unsigned int index = hash_function(key, table->size);

    FlagNode* current = table->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value; // Key found
        }
        current = current->next;
    }

    return NULL; // Key not found
}
