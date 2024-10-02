#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define LRU_CACHE_ENTRY_NIL UINT32_MAX

struct lru_cache_entry {
    // less recently used, more recently used
    uint32_t lru;
    uint32_t mru;
    uint32_t clru;
    uint32_t cmru;

    char key[];
};

typedef void (*lru_cache_destroy_t)(void *a);
typedef int (*lru_cache_compare_t)(const void *a, const void *b);
typedef uint32_t (*lru_cache_hash_t)(const void *a);

struct lru_cache {
    uint32_t size;
    uint32_t nmemb;

    // leat recently used, most recently used
    uint32_t lru, mru;

    uint32_t *hashmap;
    void *cache;

    lru_cache_hash_t hash;
    lru_cache_compare_t compare;
    lru_cache_destroy_t destroy;
};

/** Initializes the size and nmemb fields of the cache. Returns how much memory should be allocated in hashmap_bytes and cache_bytes for the hashmap and cache respectively.
 *
 * @param size Size of each cache entry
 * @param nmemb Number of cache entries
 * @param align Address alignment 1 2 4 8 or 16 of each cache entry
 * @param hashmap_bytes Number of bytes required for hashmap memory
 * @param cache_bytes Number of bytes required for lru cache
 */
int lru_cache_init_size(struct lru_cache *s, uint32_t size, uint32_t nmemb, uint32_t align, size_t *hashmap_bytes, size_t *cache_bytes);

/** Initializes the m
 */
int lru_cache_init_memory(struct lru_cache *s, void *hashmap, void *cache, lru_cache_hash_t hash, lru_cache_compare_t compare, lru_cache_destroy_t destroy);

struct lru_cache_entry *lru_cache_get_entry(struct lru_cache *s, uint32_t i);

bool lru_cache_get_or_put(struct lru_cache *s, const void *key, uint32_t *index);
void lru_cache_flush(struct lru_cache *s);

#endif // LRU_CACHE_H_
