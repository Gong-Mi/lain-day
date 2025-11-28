#ifndef CMAP_H
#define CMAP_H

// Forward declaration of the Location struct to break circular dependency.
// We only need the pointer type here, not the full definition.
struct Location;

// A node in the hash table's chain
typedef struct MapNode {
    char* key;              // The location ID (e.g., "lain_room")
    struct Location* value; // Pointer to the actual Location struct
    struct MapNode* next;   // Pointer to the next node in case of collision
} MapNode;

// The hash table itself
typedef struct {
    int size;           // The number of buckets in the hash table
    MapNode** buckets;  // An array of pointers to MapNode
} CMap;

/**
 * @brief Creates a new CMap (hash table for locations).
 * 
 * @param size The number of buckets for the hash table.
 * @return A pointer to the newly created CMap, or NULL on failure.
 */
CMap* cmap_create(int size);

/**
 * @brief Inserts a Location into the CMap.
 * 
 * @param cmap A pointer to the CMap.
 * @param location A pointer to the Location struct to insert. The CMap does NOT take ownership of this pointer.
 */
void cmap_insert(CMap* cmap, struct Location* location);

/**
 * @brief Retrieves a Location from the CMap by its ID.
 * 
 * @param cmap A pointer to the CMap.
 * @param location_id The ID of the location to retrieve.
 * @return A pointer to the Location struct, or NULL if not found.
 */
struct Location* cmap_get(CMap* cmap, const char* location_id);

/**
 * @brief Destroys the CMap and frees associated memory.
 * 
 * @param cmap A pointer to the CMap to destroy.
 */
void cmap_destroy(CMap* cmap);

#endif // CMAP_H
