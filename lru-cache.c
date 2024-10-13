#include "lru-cache.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <memory.h>
#include <errno.h>

static void lru_cache_pop(struct lru_cache *s, struct lru_cache_entry *e)
{
    if (e->lru != LRU_CACHE_ENTRY_NIL) {
        lru_cache_get_entry(s, e->lru)->mru = e->mru;
    } else {
        s->lru = e->mru;
    }

    if (e->mru != LRU_CACHE_ENTRY_NIL) {
        lru_cache_get_entry(s, e->mru)->lru = e->lru;
    } else {
        s->mru = e->lru;
    }
}

static void lru_cache_cpop(
    struct lru_cache *s,
    uint32_t i,
    struct lru_cache_entry *e,
    uint32_t old_hash)
{
    if (e->clru != LRU_CACHE_ENTRY_NIL) {
        lru_cache_get_entry(s, e->clru)->cmru = e->cmru;
    } else {
        // no lru pointer in collision chain
    }

    if (e->cmru != LRU_CACHE_ENTRY_NIL) {
        lru_cache_get_entry(s, e->cmru)->clru = e->clru;
    } else if (e->clru != i) {
        s->hashmap[old_hash] = e->clru;
    }
}

static uint32_t lru_cache_update_entry(
    struct lru_cache *s,
    uint32_t i,
    struct lru_cache_entry *e,
    uint32_t old_hash,
    uint32_t new_hash)
{
    // Make current entry most recently used in the global chain if not already
    if (s->mru != i) {
        lru_cache_pop(s, e);
        lru_cache_get_entry(s, s->mru)->mru = i;

        e->lru = s->mru;
        s->mru = i;

        e->mru = LRU_CACHE_ENTRY_NIL;
    }

    // Make current entry most recently used in the local chain if not already
    if (s->hashmap[new_hash] != i) {
        lru_cache_cpop(s, i, e, old_hash);

        e->clru = s->hashmap[new_hash];
        e->cmru = LRU_CACHE_ENTRY_NIL;

        if (e->clru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->clru)->cmru = i;
        }

        s->hashmap[new_hash] = i;
    }

    assert((e->clru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->clru)->cmru == i));
    assert((e->cmru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->cmru)->clru == i));

    assert((e->lru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->lru)->mru == i));
    assert((e->mru == LRU_CACHE_ENTRY_NIL) || (lru_cache_get_entry(s, e->mru)->lru == i));

    assert(s->lru != LRU_CACHE_ENTRY_NIL);
    assert(s->mru == i);

    return i;
}

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

int lru_cache_init(
    struct lru_cache *s,
    uint32_t size,
    uint32_t align,
    lru_cache_hash_t hash,
    lru_cache_compare_t compare,
    lru_cache_destroy_t destroy)
{
    uint32_t aligned_size = (size + align - 1) & ~(align - 1);

    if (compare == NULL || hash == NULL) {
        return EINVAL;
    }

    if (size == 0 || align == 0 || align > sizeof(struct lru_cache_entry)) {
        return EINVAL;
    }

    if (aligned_size < size) {
        return EOVERFLOW;
    }

    s->destroy = destroy;
    s->compare = compare;
    s->hash = hash;

    s->size = aligned_size;
    s->nmemb = 0;

    s->lru = LRU_CACHE_ENTRY_NIL;
    s->mru = LRU_CACHE_ENTRY_NIL;
    return 0;
}

int lru_cache_set_nmemb(
    struct lru_cache *s,
    uint32_t nmemb,
    size_t *hashmap_bytes,
    size_t *cache_bytes)
{
    uint32_t i, h;
    struct lru_cache_entry *e;

    uint32_t nmemb_max = SIZE_MAX / (sizeof(struct lru_cache_entry) + s->size);

    if (nmemb == 0) {
        return EINVAL;
    }

    if (nmemb > nmemb_max) {
        return EOVERFLOW;
    }

    s->old_nmemb = s->nmemb;
    s->nmemb = nmemb;

    if (s->nmemb < s->old_nmemb) {
        for (i = s->nmemb; i < s->old_nmemb; i++) {
            e = lru_cache_get_entry(s, i);
            h = s->hash(e->key);

            if (s->destroy) {
                s->destroy(e->key, i);
            }

            lru_cache_pop(s, e);
            lru_cache_cpop(s, i, e, h % s->old_nmemb);
        }
    }

    if (hashmap_bytes) {
        *hashmap_bytes = s->nmemb * sizeof(*s->hashmap);
    }

    if (cache_bytes) {
        *cache_bytes = s->nmemb * (sizeof(struct lru_cache_entry) + s->size);
    }

    return 0;
}

int lru_cache_set_memory(struct lru_cache *s, void *hashmap, void *cache)
{
    uint32_t i, h;
    struct lru_cache_entry *e;

    size_t hashmap_bytes = s->nmemb * sizeof(*s->hashmap);
    size_t cache_bytes = s->nmemb * (sizeof(struct lru_cache_entry) + s->size);

    if (UINTPTR_MAX - (uintptr_t)cache < cache_bytes) {
        return EOVERFLOW;
    }

    if (UINTPTR_MAX - (uintptr_t)hashmap < hashmap_bytes) {
        return EOVERFLOW;
    }

    s->hashmap = hashmap;
    s->cache = cache;

    if (s->old_nmemb < s->nmemb) {
        for (i = s->old_nmemb; i < s->nmemb; i++) {
            e = lru_cache_get_entry(s, i);

            e->lru = (i > s->old_nmemb) ? (i - 1) : LRU_CACHE_ENTRY_NIL;
            e->mru = (i < s->nmemb - 1) ? (i + 1) : s->lru;

            e->clru = i;
            e->cmru = LRU_CACHE_ENTRY_NIL;

            s->hashmap[i] = LRU_CACHE_ENTRY_NIL;
        }

        if (s->lru == LRU_CACHE_ENTRY_NIL) {
            s->lru = s->old_nmemb;
        } else {
            lru_cache_get_entry(s, s->lru)->lru = s->nmemb - 1;
        }

        if (s->mru == LRU_CACHE_ENTRY_NIL) {
            s->mru = s->nmemb - 1;
        }
    }

    for (i = s->mru; (e = lru_cache_get_entry(s, i))->clru != i; i = e->lru) {
        assert(i < s->old_nmemb);

        h = s->hash(e->key);
        lru_cache_cpop(s, i, e, h % s->old_nmemb);

        e->clru = s->hashmap[h % s->nmemb];
        e->cmru = LRU_CACHE_ENTRY_NIL;

        if (e->clru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, e->clru)->cmru = i;
        }

        s->hashmap[h % s->nmemb] = i;
    }

    s->old_nmemb = s->nmemb;
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
    uint32_t new_hash = s->hash(key);
    uint32_t old_hash = new_hash;
    uint32_t i = s->hashmap[new_hash % s->nmemb];
    struct lru_cache_entry *e = NULL;

    while (i != LRU_CACHE_ENTRY_NIL) {
        e = lru_cache_get_entry(s, i);

        if (s->compare(e->key, key) == 0) {
            if (put) {
                *put = false;
            }

            return lru_cache_update_entry(s, i, e, old_hash % s->nmemb, new_hash % s->nmemb);
        }

        i = e->clru;
    }

    if (!put) {
        return LRU_CACHE_ENTRY_NIL;
    }

    *put = true;
    i = s->lru;
    e = lru_cache_get_entry(s, i);
    old_hash = s->hash(e->key);

    if (e->clru != i && s->destroy) {
        s->destroy(e->key, i);
    }

    memcpy(e->key, key, s->size);
    return lru_cache_update_entry(s, i, e, old_hash % s->nmemb, new_hash % s->nmemb);
}

void lru_cache_flush(struct lru_cache *s)
{
    /*
     * When flushing the cache, there are three possible strategies:
     *
     * 1. Iterate through all entries, remove them, and reinsert in LRU -> MRU order (mapped to 0 -> N)
     *  + Ensures entries are inserted in order (0, 1, ..., N) for better memory locality
     *  - More costly since all entries must be removed and reinserted
     *  * Old access order is lost, which may negatively impact CPU cache performance
     *
     * 2. Iterate from 0 to N, destroying only used entries and removing them from the collision chain
     *  + Accesses entries in sequential order (0 -> N), improving CPU cache efficiency
     *  - May unnecessarily iterate over empty entries if the cache is not full
     *
     * 3. Iterate from MRU -> LRU, stopping at the first uninitialized entry
     *  + Stops early if there are unused entries, saving time
     *  - Traversal is not sequential in memory, which may reduce CPU cache efficiency
     */
    uint32_t i, h;
    struct lru_cache_entry *e;

    for (i = s->mru; (e = lru_cache_get_entry(s, i))->clru != i; i = e->lru) {
        h = s->hash(e->key);

        if (s->destroy) {
            s->destroy(e->key, i);
        }

        lru_cache_cpop(s, i, e, h % s->nmemb);
    }
}
