#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define LRU_CACHE_ENTRY_NIL UINT32_MAX

/**
 * @struct lru_cache_entry
 * @brief Structure representing an entry in the LRU cache.
 *
 * Each cache entry contains links to manage its position in both global
 * and local LRU (Least Recently Used) and MRU (Most Recently Used) chains,
 * as well as the actual key stored in the entry.
 */
struct lru_cache_entry {
    uint32_t lru; ///< Pointer to the less recently used entry in the global chain.
    uint32_t mru; ///< Pointer to the most recently used entry in the global chain.
    uint32_t clru; ///< Pointer to the less recently used entry in the local chain.
    uint32_t cmru; ///< Pointer to the most recently used entry in the local chain.
    char key[]; ///< Variable-sized key storage.
};

/**
 * Function pointer type for destroying cache entry data.
 *
 * This function is called when an entry is being evicted from the cache.
 * It provides the user with the entry index for proper resource management.
 *
 * @param a Pointer to the key or data to be destroyed.
 * @param index The index of the entry being destroyed in the cache.
 */
typedef void (*lru_cache_destroy_t)(void *a, uint32_t index);

/**
 * @typedef lru_cache_compare_t
 * @brief Function pointer type for comparing keys.
 *
 * This function determines the equality of two keys for cache lookups.
 */
typedef int (*lru_cache_compare_t)(const void *a, const void *b);

/**
 * @typedef lru_cache_hash_t
 * @brief Function pointer type for hashing keys.
 *
 * This function generates a hash value for a given key, which is used to
 * determine the index in the hashmap.
 */
typedef uint32_t (*lru_cache_hash_t)(const void *a);

/**
 * @struct lru_cache
 * @brief Structure representing the LRU cache itself.
 *
 * This structure holds the configuration of the cache, including the size
 * of each entry, the number of entries, pointers for managing the LRU/MRU
 * ordering, and references to the hashmap and cache memory.
 */
struct lru_cache {
    uint32_t *hashmap; ///< Hashmap for quick access to cache entries.
    void *cache; ///< Pointer to the cache memory.

    lru_cache_hash_t hash; ///< Hash function for the cache keys.
    lru_cache_compare_t compare; ///< Comparison function for cache keys.
    lru_cache_destroy_t destroy; ///< Function to destroy cache entries.

    uint32_t size; ///< Size of each cache entry.
    uint32_t nmemb; ///< Number of cache entries.
    uint32_t try_nmemb; //< Size requested through lru_cache_set_nmemb.

    uint32_t lru; ///< Pointer to the least recently used entry.
    uint32_t mru; ///< Pointer to the most recently used entry.
};

bool lru_cache_is_full(
    struct lru_cache *s);

int lru_cache_align(uint32_t size, uint32_t align, uint32_t *aligned_size_);

int lru_cache_init(
    struct lru_cache *s,
    uint32_t aligned_size,
    lru_cache_hash_t hash,
    lru_cache_compare_t compare,
    lru_cache_destroy_t destroy);

/**
 * @brief Sets the number of cache entries and calculates required memory sizes.
 *
 * This function configures the number of entries (`nmemb`) the cache will handle. It also calculates
 * the required memory sizes for both the hashmap and the cache. Memory for these must be allocated
 * separately and provided through `lru_cache_set_memory()`.
 *
 * If any parameter is invalid, such as `nmemb` being zero, this function returns an error without
 * modifying the cache object.
 *
 * @param s Pointer to the `lru_cache` structure to be modified.
 * @param nmemb Number of cache entries. Must be greater than 0.
 * @param hashmap_bytes Pointer to store the required bytes for the hashmap memory.
 * @param cache_bytes Pointer to store the required bytes for the cache memory.
 * @return 0 on success, or a positive error number:
 *         - EINVAL: Invalid `nmemb` value.
 *         - EOVERFLOW: Overflow detected while calculating memory requirements.
 */
int lru_cache_set_nmemb(
    struct lru_cache *s,
    uint32_t nmemb,
    size_t *hashmap_bytes,
    size_t *cache_bytes);

/**
 * @brief Initializes the memory for the cache using external allocation.
 *
 * This function links the externally allocated memory for both the hashmap and cache storage
 * to the LRU cache structure. It ensures that the cache is set up correctly by associating
 * the hash, comparison, and destroy functions for key management. The cache starts in an empty state,
 * ready to be populated.
 *
 * The memory size for the hashmap and cache must match the sizes computed by `lru_cache_set_nmemb()`.
 * If any parameter is invalid, the function returns an error and does not modify the cache.
 *
 * @param s Pointer to the `lru_cache` structure to be modified.
 * @param hashmap Pointer to the allocated hashmap memory.
 * @param cache Pointer to the allocated cache memory.
 * @return 0 on success, or a positive error number:
 *         - EINVAL: Invalid pointers or unaligned memory.
 */
int lru_cache_set_memory(
    struct lru_cache *s,
    void *hashmap,
    void *cache);

/**
 * @brief Retrieves a pointer to a cache entry at a given index.
 *
 * If the index is invalid (equal to LRU_CACHE_ENTRY_NIL), the function returns NULL.
 *
 * @param s Pointer to the lru_cache structure.
 * @param i Index of the desired cache entry; should be a valid index within the cache.
 * @return Pointer to the lru_cache_entry if the index is valid, or NULL if invalid.
 */
struct lru_cache_entry *lru_cache_get_entry(
    struct lru_cache *s,
    uint32_t i);

/**
 * @brief Retrieves or inserts a cache entry based on the provided key.
 *
 * This function attempts to retrieve a cache entry based on the given key. If the key is found,
 * it updates the entry's position in the cache to make it the most recently used (MRU) and
 * returns the entry's index. If the key is not found and the `put` parameter is non-NULL, a new
 * entry is created and marked as MRU. If the cache is full, the least recently used (LRU) entry
 * is evicted. The `destroy` function will be called for the evicted entry if provided.
 *
 * If the `put` parameter is NULL, the function behaves as a "get" operation and does not modify
 * the cache. It will return the index of the found entry or `LRU_CACHE_ENTRY_NIL` if the key is
 * not found.
 *
 * The `put` boolean pointer, if non-NULL, will indicate whether a new entry was inserted (`true`)
 * or the key was found (`false`).
 *
 * @remark If an error occurs, the cache object remains unchanged.
 *
 * @param s Pointer to the lru_cache structure. Must not be NULL.
 * @param key Pointer to the key to be searched for or inserted; must not be NULL.
 * @param put Pointer to a boolean that will be set to `true` if a new entry was inserted, or
 *            `false` if the key was found. If NULL, the function only performs a lookup.
 * @return The index of the found or newly inserted cache entry. If the key is not found and `put`
 *         is NULL, `LRU_CACHE_ENTRY_NIL` is returned.
 */
uint32_t lru_cache_get_or_put(
    struct lru_cache *s,
    const void *key,
    bool *put);

/**
 * @brief Flushes the entire cache, destroying all entries.
 *
 * This function iterates through all entries in the cache and calls the destroy
 * function for each entry if it is provided. After clearing the cache, it resets
 * the global LRU and MRU pointers, preparing the cache for future use.
 *
 * @param s Pointer to the lru_cache structure.
 */
void lru_cache_flush(
    struct lru_cache *s);

void lru_cache_print(
    struct lru_cache *s,
    FILE *file);

#endif // LRU_CACHE_H_
