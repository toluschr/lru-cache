#include "lru-cache.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <memory.h>
#include <errno.h>

void lru_cache_print(struct lru_cache *s, FILE *file)
{
    uint32_t i;
    bool visited[s->nmemb];
    uint32_t nvisited = 0;

    memset(visited, 0, sizeof(visited));

    for (i = 0; i < s->nmemb; i++) {
        fprintf(file, "[%d]", i);

        uint32_t entry_id = s->hashmap[i];
        struct lru_cache_entry *entry = NULL;

        while (entry_id != LRU_CACHE_ENTRY_NIL) {
            if (visited[entry_id]) {
                fprintf(file, " --> %d ((CORRUPTION))", entry_id);
                break;
            }

            visited[entry_id] = true;
            nvisited++;

            entry = lru_cache_get_entry(s, entry_id);

            fprintf(file, " --> %d", entry_id);

            entry_id = entry->clru;
        }

        fprintf(file, "\n");
    }

    i = s->mru;

    fprintf(file, "Unused ML");
    while (i != LRU_CACHE_ENTRY_NIL) {
        struct lru_cache_entry *e = lru_cache_get_entry(s, i);

        if (e->clru == i) {
            if (visited[i]) {
                fprintf(file, " --> %d ((CORRUPTION))", i);
                break;
            }

            visited[i] = true;
            nvisited++;
            fprintf(file, " --> %d", i);
        }

        i = e->lru;
    }
    fprintf(file, "\n");

    fprintf(file, "LRU: %d\n", s->lru);
    fprintf(file, "MRU: %d\n", s->mru);

    if (nvisited != s->nmemb) {
        fprintf(file, "%d entries are unlinked ((CORRUPTION))\n", s->nmemb - nvisited);
    }
}

int lru_cache_init_size(struct lru_cache *s, uint32_t size, uint32_t nmemb, uint32_t align, size_t *hashmap_bytes, size_t *cache_bytes)
{
    uint32_t aligned_size = (size + align - 1) & ~(align - 1);
    uint32_t nmemb_max = SIZE_MAX / (sizeof(struct lru_cache_entry) + aligned_size);

    if (size == 0 || nmemb == 0 || align == 0 || align > sizeof(struct lru_cache_entry)) {
        return EINVAL;
    }

    if (aligned_size < size) {
        return EOVERFLOW;
    }

    if (s->nmemb > nmemb_max) {
        return EOVERFLOW;
    }

    s->size = aligned_size;
    s->nmemb = nmemb;

    if (hashmap_bytes) {
        *hashmap_bytes = s->nmemb * sizeof(uint32_t);
    }

    if (cache_bytes) {
        *cache_bytes = s->nmemb * (sizeof(struct lru_cache_entry) + s->size);
    }

    return 0;
}

int lru_cache_init_memory(struct lru_cache *s, void *hashmap, void *cache, lru_cache_hash_t hash, lru_cache_compare_t compare, lru_cache_destroy_t destroy)
{
    size_t hashmap_bytes = s->nmemb * sizeof(uint32_t);
    size_t cache_bytes = s->nmemb * (sizeof(struct lru_cache_entry) + s->size);

    if (compare == NULL || hash == NULL) {
        return EINVAL;
    }

    if (UINTPTR_MAX - (uintptr_t)cache < cache_bytes) {
        return EOVERFLOW;
    }

    if (UINTPTR_MAX - (uintptr_t)hashmap < hashmap_bytes) {
        return EOVERFLOW;
    }

    s->hashmap = hashmap;
    s->cache = cache;

    s->destroy = destroy;
    s->compare = compare;
    s->hash = hash;

    // cache is empty
    s->lru = LRU_CACHE_ENTRY_NIL;
    lru_cache_flush(s);

    return 0;
}

struct lru_cache_entry *lru_cache_get_entry(struct lru_cache *s, uint32_t i)
{
    char *cache = s->cache;
    size_t offset = i * (sizeof(struct lru_cache_entry) + s->size);
    return (i != LRU_CACHE_ENTRY_NIL) ? (struct lru_cache_entry *)(cache + offset) : NULL;
}

// @todo: Atomic access
uint32_t lru_cache_get_or_put(struct lru_cache *s, const void *key, bool *put)
{
    uint32_t hash = s->hash(key);
    uint32_t old_hash = hash;

    uint32_t i = s->hashmap[hash % s->nmemb];

    struct lru_cache_entry *e = NULL;

    while (i != LRU_CACHE_ENTRY_NIL) {
        e = lru_cache_get_entry(s, i);

        if (s->compare(e->key, key) == 0) {
            if (put) {
                *put = false;
            }

            goto access_and_rearrange;
        }

        i = e->clru;
    }

    if (!put) {
        return LRU_CACHE_ENTRY_NIL;
    }

    // remove from collision chain
    *put = true;

    i = s->lru;
    e = lru_cache_get_entry(s, i);

    old_hash = s->hash(e->key);

    if (e->clru != i && s->destroy) {
        s->destroy(e->key, i);
    }

    memcpy(e->key, key, s->size);

access_and_rearrange:
    // Make current entry most recently used in the global chain if not already
    if (s->mru != i) {
        if (e->lru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->lru)->mru = e->mru;
        } else {
            s->lru = lru_cache_get_entry(s, s->lru)->mru;
        }

        if (e->mru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->mru)->lru = e->lru;
        }

        lru_cache_get_entry(s, s->mru)->mru = i;

        e->lru = s->mru;
        s->mru = i;

        e->mru = LRU_CACHE_ENTRY_NIL;
    }

    // Make current entry most recently used in the local chain if not already
    if (s->hashmap[hash % s->nmemb] != i) {
        if (e->clru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->clru)->cmru = e->cmru;
        }

        if (e->cmru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->cmru)->clru = e->clru;
        } else if (e->clru != i) {
            s->hashmap[old_hash % s->nmemb] = e->clru;
        }

        e->clru = s->hashmap[hash % s->nmemb];
        e->cmru = LRU_CACHE_ENTRY_NIL;

        if (e->clru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->clru)->cmru = i;
        }

        s->hashmap[hash % s->nmemb] = i;
    }

    assert((e->clru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->clru)->cmru == i));
    assert((e->cmru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->cmru)->clru == i));

    assert((e->lru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->lru)->mru == i));
    assert((e->mru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->mru)->lru == i));

    assert(s->lru != LRU_CACHE_ENTRY_NIL);
    assert(s->mru == i);

    return i;
}

void lru_cache_flush(struct lru_cache *s)
{
    uint32_t i = s->lru;
    struct lru_cache_entry *e;

    while (s->destroy && i != LRU_CACHE_ENTRY_NIL) {
        e = lru_cache_get_entry(s, i);

        if (e->clru != i) {
            s->destroy(e->key, i);
        }

        i = e->mru;
    }

    s->lru = 0;
    s->mru = s->nmemb - 1;

    for (i = 0; i < s->nmemb; i++) {
        e = lru_cache_get_entry(s, i);

        e->lru = (i > 0) ? (i - 1) : LRU_CACHE_ENTRY_NIL;
        e->mru = (i < s->nmemb - 1) ? (i + 1) : LRU_CACHE_ENTRY_NIL;

        e->clru = i;
        e->cmru = LRU_CACHE_ENTRY_NIL;

        s->hashmap[i] = LRU_CACHE_ENTRY_NIL;
    }
}
