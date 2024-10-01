#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

struct lru_cache_entry {
    // less recently used, more recently used
    uint32_t lru, mru;
    uint32_t clru, cmru;
    char key[];
};

typedef void (*lru_cache_destroy_t)(void *a, size_t l);
typedef int (*lru_cache_compare_t)(const void *a, const void *b, size_t l);
typedef uint32_t (*lru_cache_hash_t)(const void *a, size_t l);

struct lru_cache {
    uint32_t size;
    uint32_t nmemb;

    // leat recently used, most recently used
    uint32_t lru, mru;

    uint32_t *hashmap;
    void *cache;

    lru_cache_destroy_t destroy;
    lru_cache_compare_t compare;
    lru_cache_hash_t hash;
};

uint32_t lru_cache_get_entry_offset(uint32_t index, uint32_t size);

bool lru_cache_get_or_put(struct lru_cache *s, const void *key, uint32_t *index);

void lru_cache_flush(struct lru_cache *s);

#endif // LRU_CACHE_H_
