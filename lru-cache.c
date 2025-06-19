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

    e->mru = LRU_CACHE_ENTRY_NIL;
    e->lru = LRU_CACHE_ENTRY_NIL;
}

static void lru_cache_rehash(
    struct lru_cache *s,
    uint32_t i,
    struct lru_cache_entry *e,
    uint32_t old_hash,
    uint32_t new_hash)
{
    uint32_t *index;

    if (new_hash == LRU_CACHE_ENTRY_NIL) {
        index = &i;
    } else if (s->hashmap[new_hash] != i) {
        index = &s->hashmap[new_hash];
    } else {
        return;
    }

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

    // If removal requested, "*index == i" => no longer in any chain.
    e->clru = *index;

    // If relocation requested, "s->hashmap[new_hash]->cmru = i" => first element in new chain.
    if (e->clru != LRU_CACHE_ENTRY_NIL) {
        lru_cache_get_entry(s, e->clru)->cmru = i;
    }

    // Element is the most used in both cases
    e->cmru = LRU_CACHE_ENTRY_NIL;

    // If relocation requested: "s->hashmap[new_hash] = i" => first element in new chain.
    // If removal requested: "i = i" => NOP
    *index = i;
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

    lru_cache_rehash(s, i, e, old_hash, new_hash);

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

    memset(visited, 0, sizeof(visited));
    nvisited = 0;

    i = s->lru;

    fprintf(file, "LRU chain");
    while (i != LRU_CACHE_ENTRY_NIL) {
        struct lru_cache_entry *e = lru_cache_get_entry(s, i);

        visited[i] = true;
        nvisited++;
        fprintf(file, " --> %d", i);

        i = e->mru;
    }
    fprintf(file, "\n");

    fprintf(file, "LRU: %d\n", s->lru);
    fprintf(file, "MRU: %d\n", s->mru);

    if (nvisited != s->nmemb) {
        fprintf(file, "%d entries are unlinked ((CORRUPTION))\n", s->nmemb - nvisited);
    }

    fprintf(file, "\n");
}

int lru_cache_align(uint32_t size, uint32_t align, uint32_t *aligned_size_)
{
    uint32_t aligned_size = (size + align - 1) & ~(align - 1);

    if (size == 0 || align == 0 || align > sizeof(struct lru_cache_entry)) {
        return EINVAL;
    }

    if (aligned_size < size) {
        return EOVERFLOW;
    }

    if (aligned_size_) {
        *aligned_size_ = aligned_size;
    }

    return 0;
}

int lru_cache_init(
    struct lru_cache *s,
    uint32_t aligned_size,
    lru_cache_hash_t hash,
    lru_cache_compare_t compare,
    lru_cache_destroy_t destroy)
{
    if (aligned_size == 0) {
        return EINVAL;
    }

    if (compare == NULL || hash == NULL) {
        return EINVAL;
    }

    s->hashmap = NULL;
    s->cache = NULL;

    s->destroy = destroy;
    s->compare = compare;
    s->hash = hash;

    s->size = aligned_size;
    s->nmemb = 0;

    s->lru = LRU_CACHE_ENTRY_NIL;
    s->mru = LRU_CACHE_ENTRY_NIL;
    return 0;
}

bool lru_cache_is_full(
    struct lru_cache *s)
{
    struct lru_cache_entry *entry = lru_cache_get_entry(s, s->lru);
    return (entry == NULL) || (entry->clru != s->lru);
}

int lru_cache_calc_sizes(size_t aligned_size, size_t nmemb, size_t *hashmap_bytes, size_t *cache_bytes)
{
    uint32_t nmemb_max = SIZE_MAX / (sizeof(struct lru_cache_entry) + aligned_size);

    if (nmemb == 0) {
        return EINVAL;
    }

    if (nmemb > nmemb_max) {
        return EOVERFLOW;
    }

    if (hashmap_bytes) {
        *hashmap_bytes = nmemb * sizeof(uint32_t);
    }

    if (cache_bytes) {
        *cache_bytes = nmemb * (sizeof(struct lru_cache_entry) + aligned_size);
    }

    return 0;
}

int lru_cache_set_nmemb(
    struct lru_cache *s,
    uint32_t nmemb,
    size_t *hashmap_bytes,
    size_t *cache_bytes)
{
    int rv = 0;
    uint32_t i;
    uint32_t old_hash;
    uint32_t new_hash;
    struct lru_cache_entry *e;

    rv = lru_cache_calc_sizes(s->size, nmemb, hashmap_bytes, cache_bytes);
    if (rv != 0) {
        return rv;
    }

    if (nmemb < s->nmemb) {
        for (i = nmemb; i < s->nmemb; i++) {
            e = lru_cache_get_entry(s, i);
            old_hash = s->hash(e->key, s->nmemb);

            if (e->clru != i && s->destroy) {
                s->destroy(e->key, i);
            }

            lru_cache_pop(s, e);
            lru_cache_rehash(s, i, e, old_hash, LRU_CACHE_ENTRY_NIL);
        }

        assert(s->lru < nmemb);
        assert(s->mru < nmemb);

        for (i = s->mru; (e = lru_cache_get_entry(s, i)) && e->clru != i; i = e->lru) {
            assert(i < s->nmemb);

            old_hash = s->hash(e->key, s->nmemb);
            new_hash = s->hash(e->key, nmemb);
            lru_cache_rehash(s, i, e, old_hash, new_hash);
        }

        s->nmemb = nmemb;
    }

    s->try_nmemb = nmemb;
    return rv;
}

int lru_cache_set_memory(struct lru_cache *s, void *hashmap, void *cache)
{
    uint32_t i;
    uint32_t old_hash;
    uint32_t new_hash;
    struct lru_cache_entry *e;

    size_t hashmap_bytes = s->try_nmemb * sizeof(*s->hashmap);
    size_t cache_bytes = s->try_nmemb * (sizeof(struct lru_cache_entry) + s->size);

    if (UINTPTR_MAX - (uintptr_t)cache < cache_bytes) {
        return EOVERFLOW;
    }

    if (UINTPTR_MAX - (uintptr_t)hashmap < hashmap_bytes) {
        return EOVERFLOW;
    }

    s->hashmap = hashmap;
    s->cache = cache;

    if (s->nmemb < s->try_nmemb) {
        for (i = s->nmemb; i < s->try_nmemb; i++) {
            e = lru_cache_get_entry(s, i);

            e->lru = (i > s->nmemb) ? (i - 1) : LRU_CACHE_ENTRY_NIL;
            e->mru = (i < s->try_nmemb - 1) ? (i + 1) : s->lru;

            /*
             * The member clru of a cache entry is tri-state:
             *
             * 1. Not in a collision chain (e->clru == i)
             * 2. The last element of a collision chain (e->clru == LRU_CACHE_ENTRY_NIL)
             * 3. Any other element of a collision chain (e->clru != i && e->clru != lru_cache_entry_nil)
             */
            e->clru = i;
            e->cmru = LRU_CACHE_ENTRY_NIL;

            s->hashmap[i] = LRU_CACHE_ENTRY_NIL;
        }

        if (s->lru != LRU_CACHE_ENTRY_NIL) {
            lru_cache_get_entry(s, s->lru)->lru = s->try_nmemb - 1;
        }

        s->lru = s->nmemb;

        if (s->mru == LRU_CACHE_ENTRY_NIL) {
            s->mru = s->try_nmemb - 1;
        }

        for (i = s->mru; (e = lru_cache_get_entry(s, i)) && e->clru != i; i = e->lru) {
            assert(i < s->nmemb && i < s->try_nmemb);

            old_hash = s->hash(e->key, s->nmemb);
            new_hash = s->hash(e->key, s->try_nmemb);
            lru_cache_rehash(s, i, e, old_hash, new_hash);
        }

        s->nmemb = s->try_nmemb;
    }

    // guaranteed by correct order of set_nmemb and set_memory
    assert(s->nmemb == s->try_nmemb);
    return 0;
}

struct lru_cache_entry *lru_cache_get_entry(struct lru_cache *s, uint32_t i)
{
    char *cache = s->cache;
    size_t offset = i * (sizeof(struct lru_cache_entry) + s->size);
    return (i != LRU_CACHE_ENTRY_NIL) ? (struct lru_cache_entry *)(cache + offset) : NULL;
}

uint32_t lru_cache_put(struct lru_cache *s, const void *key)
{
    uint32_t i = s->lru;
    uint32_t new_hash = s->hash(key, s->nmemb);
    uint32_t old_hash = new_hash;
    struct lru_cache_entry *e = lru_cache_get_entry(s, i);

    if (e->clru != i && s->destroy) {
        old_hash = s->hash(e->key, s->nmemb);
        s->destroy(e->key, i);
    }

    memcpy(e->key, key, s->size);
    return lru_cache_update_entry(s, i, e, old_hash, new_hash);
}

// @todo: Atomic access
uint32_t lru_cache_get_or_put(struct lru_cache *s, const void *key, bool *put)
{
    uint32_t new_hash = s->hash(key, s->nmemb);
    uint32_t old_hash = new_hash;
    uint32_t i = s->hashmap[new_hash % s->nmemb];
    struct lru_cache_entry *e = NULL;

    while ((e = lru_cache_get_entry(s, i))) {
        if (s->compare(e->key, key) == 0) {
            if (put) {
                *put = false;
            }

            return lru_cache_update_entry(s, i, e, old_hash, new_hash);
        }

        i = e->clru;
    }

    if (!put) {
        return LRU_CACHE_ENTRY_NIL;
    }

    *put = true;
    return lru_cache_put(s, key);
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
    uint32_t i;
    uint32_t old_hash;
    struct lru_cache_entry *e;

    for (i = s->mru; (e = lru_cache_get_entry(s, i)) && e->clru != i; i = e->lru) {
        old_hash = s->hash(e->key, s->nmemb);

        if (s->destroy) {
            s->destroy(e->key, i);
        }

        lru_cache_rehash(s, i, e, old_hash, LRU_CACHE_ENTRY_NIL);
    }
}
