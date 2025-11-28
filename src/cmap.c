#include "cmap.h"
#include "game_types.h" // For the full definition of Location
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Private Functions ---

/**
 * @brief Hash function (djb2) for a string key.
 * @param str The string to hash.
 * @return The calculated hash value.
 */
static unsigned long hash_string(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// --- Public API Implementation ---

CMap* cmap_create(int size) {
    if (size <= 0) {
        return NULL;
    }

    CMap* cmap = malloc(sizeof(CMap));
    if (!cmap) {
        return NULL;
    }

    cmap->size = size;
    cmap->buckets = calloc(size, sizeof(MapNode*)); // calloc initializes to NULL
    if (!cmap->buckets) {
        free(cmap);
        return NULL;
    }

    return cmap;
}

void cmap_insert(CMap* cmap, struct Location* location) {
    if (!cmap || !location) { // Removed !location->id check as it's an array and always true
        return;
    }

    unsigned long hash = hash_string(((Location*)location)->id);
    int index = hash % cmap->size;

    // Create a new node
    MapNode* new_node = malloc(sizeof(MapNode));
    if (!new_node) {
        // In a real-world scenario, handle this more gracefully
        fprintf(stderr, "Error: Failed to allocate memory for CMap node.\n");
        return;
    }
    
    // The key needs to be duplicated as the original location->id might change
    // or the location struct might be freed elsewhere.
    new_node->key = strdup(((Location*)location)->id); 
    new_node->value = (struct Location*)location; // Store pointer to the actual location
    new_node->next = NULL;

    // Insert into the bucket (handle collision by chaining)
    if (cmap->buckets[index] == NULL) {
        cmap->buckets[index] = new_node;
    } else {
        // Add to the front of the list
        new_node->next = cmap->buckets[index];
        cmap->buckets[index] = new_node;
    }
}

struct Location* cmap_get(CMap* cmap, const char* location_id) {
    if (!cmap || !location_id) {
        return NULL;
    }

    unsigned long hash = hash_string(location_id);
    int index = hash % cmap->size;

    MapNode* current = cmap->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, location_id) == 0) {
            return (struct Location*)current->value;
        }
        current = current->next;
    }

    return NULL; // Not found
}

void cmap_destroy(CMap* cmap) {
    if (!cmap) {
        return;
    }

    for (int i = 0; i < cmap->size; i++) {
        MapNode* current = cmap->buckets[i];
        while (current != NULL) {
            MapNode* temp = current;
            current = current->next;
            free(temp->key); // Free the duplicated key
            free(temp);      // Free the node itself
        }
    }

    free(cmap->buckets);
    free(cmap);
}
