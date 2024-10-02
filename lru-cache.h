#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

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
    uint32_t size; ///< Size of each cache entry.
    uint32_t nmemb; ///< Number of cache entries.

    uint32_t lru; ///< Pointer to the least recently used entry.
    uint32_t mru; ///< Pointer to the most recently used entry.

    uint32_t *hashmap; ///< Hashmap for quick access to cache entries.
    void *cache; ///< Pointer to the cache memory.

    lru_cache_hash_t hash; ///< Hash function for the cache keys.
    lru_cache_compare_t compare; ///< Comparison function for cache keys.
    lru_cache_destroy_t destroy; ///< Function to destroy cache entries.
};

/**
 * @brief Initializes the size and nmemb fields of the cache.
 *
 * This function calculates the amount of memory required for the hashmap
 * and the cache itself based on the provided sizes, ensuring that the
 * parameters are valid.
 *
 * If any of the parameters are invalid, this function will not modify
 * the state of the cache object.
 *
 * @param s Pointer to the lru_cache object to be modified.
 * @param size Size of each cache entry. Must be greater than 0.
 * @param nmemb Number of cache entries. Must be greater than 0.
 * @param align Address alignment (1, 2, 4, 8, or 16) of each cache entry.
 * @param hashmap_bytes Pointer to store the number of bytes required for hashmap memory.
 * @param cache_bytes Pointer to store the number of bytes required for LRU cache.
 * @return 0 on success, or a positive error number:
 *         - EINVAL: Invalid input parameters (size, nmemb, or align).
 *         - EOVERFLOW: Potential overflow detected when calculating memory requirements.
 */
int lru_cache_init_size(struct lru_cache *s,
                        uint32_t size,
                        uint32_t nmemb,
                        uint32_t align,
                        size_t *hashmap_bytes,
                        size_t *cache_bytes);

/**
 * @brief Initializes the memory for the LRU cache.
 *
 * Links the provided hashmap and cache memory to the lru_cache structure,
 * and sets the hash, compare, and destroy functions. The cache starts in
 * an empty state, ready for use.
 *
 * If any of the parameters are invalid, this function will not modify
 * the state of the cache object.
 *
 * @param s Pointer to the lru_cache object to be modified.
 * @param hashmap Pointer to the allocated memory for the hashmap
 *                (at least hashmap_bytes bytes).
 * @param cache Pointer to the allocated memory for the cache
 *              (at least cache_bytes bytes).
 * @param hash Hash function to be used for the cache; determines how keys are distributed.
 * @param compare Comparison function to compare keys; used to check for key equality.
 * @param destroy Function to destroy entries when evicted from the cache; can be NULL.
 * @return 0 on success, or a positive error number:
 *         - EINVAL: Invalid comparison or hash functions.
 *         - EOVERFLOW: Memory overflow detected when initializing.
 */
int lru_cache_init_memory(struct lru_cache *s,
                          void *hashmap,
                          void *cache,
                          lru_cache_hash_t hash,
                          lru_cache_compare_t compare,
                          lru_cache_destroy_t destroy);

/**
 * @brief Retrieves a pointer to a cache entry at a given index.
 *
 * If the index is invalid (equal to LRU_CACHE_ENTRY_NIL), the function returns NULL.
 *
 * @param s Pointer to the lru_cache structure.
 * @param i Index of the desired cache entry; should be a valid index within the cache.
 * @return Pointer to the lru_cache_entry if the index is valid, or NULL if invalid.
 */
struct lru_cache_entry *lru_cache_get_entry(struct lru_cache *s,
                                            uint32_t i);

/**
 * @brief Retrieves or inserts a cache entry based on the provided key.
 *
 * If the key already exists in the cache, this function updates its position to
 * the most recently used (MRU) and returns true. If the key does not exist,
 * a new entry is created; if the cache is full, the least recently used (LRU)
 * entry is evicted.
 *
 * If the entry is evicted, the destroy function will be called for the
 * evicted entry if it is provided.
 *
 * @param s Pointer to the lru_cache structure.
 * @param key Pointer to the key to be inserted or searched for; must not be NULL.
 * @param index Pointer to store the index of the found or newly inserted entry; may be NULL.
 * @return true if the entry was found (existing), false if it was newly created.
 */
bool lru_cache_get_or_put(struct lru_cache *s,
                          const void *key,
                          uint32_t *index);

/**
 * @brief Flushes the entire cache, destroying all entries.
 *
 * This function iterates through all entries in the cache and calls the destroy
 * function for each entry if it is provided. After clearing the cache, it resets
 * the global LRU and MRU pointers, preparing the cache for future use.
 *
 * @param s Pointer to the lru_cache structure.
 */
void lru_cache_flush(struct lru_cache *s);

#endif // LRU_CACHE_H_
