#include "lru-cache.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <memory.h>
#include <errno.h>

static void
unlink(cm_cache *cm, cm_entry *e)
{
    if (e->lru != CM_NIL) {
        cm_entry_ptr(cm, e->lru)->mru = e->mru;
    } else {
        cm->lru = e->mru;
    }

    if (e->mru != CM_NIL) {
        cm_entry_ptr(cm, e->mru)->lru = e->lru;
    } else {
        cm->mru = e->lru;
    }

    e->mru = CM_NIL;
    e->lru = CM_NIL;
}

uint64_t
cm_fnv1a64_step(uint64_t state, const void *data, size_t size)
{
    const unsigned char *d = (const unsigned char *)data;
    while (size--) state = (state ^ *d++) * 0x00000100000001b3ull;
    return state;
}

uint64_t
cm_djb2_step(uint64_t state, const void *data, size_t size)
{
    const unsigned char *d = (const unsigned char *)data;
    while (size--) state = ((state << 5) + state) + *d++;
    return state;
}

int
cm_align(uint32_t size, uint32_t align, uint32_t *size_a_)
{
    uint32_t size_a = (size + align - 1) & ~(align - 1);

    if (size_a_) {
        *size_a_ = size_a;
    }

    if (size_a < size) {
        return EOVERFLOW;
    }

    if (size == 0 || align == 0 || align > sizeof(cm_entry)) {
        return EINVAL;
    }

    return 0;
}

void
check_chain_invariants(cm_cache *cm, uint32_t hash)
{
    assert(hash == CM_NIL || cm_entry_ptr(cm, cm->hashmap[hash])->clru == CM_NIL);
}

void
check_invariants(cm_cache *cm)
{
    // The global chain is valid
    uint32_t visit_count = 0;
    bool visited[cm->nmemb];
    memset(visited, 0, sizeof(visited));

    uint32_t i = cm->mru;
    cm_entry *e;
    while ((e = cm_entry_ptr(cm, i))) {
        assert(e->lru == CM_NIL || cm_entry_ptr(cm, e->lru)->mru == i);
        assert(e->mru == CM_NIL || cm_entry_ptr(cm, e->mru)->lru == i);
        assert(!visited[i]);
        visit_count += (visited[i] |= 1);
        i = e->lru;
    }

    assert(cm->mru == CM_NIL || cm_entry_ptr(cm, cm->mru)->mru == CM_NIL);
    assert(cm->lru == CM_NIL || cm_entry_ptr(cm, cm->lru)->lru == CM_NIL);
}

int
cm_required_bytes(uint32_t size_a, uint32_t nmemb, size_t *hashmap_bytes, size_t *cache_bytes)
{
    uint32_t nmemb_max = SIZE_MAX / (sizeof(cm_entry) + size_a);

    if (nmemb == 0) {
        return EINVAL;
    }

    if (nmemb > nmemb_max) {
        return EOVERFLOW;
    }

    if (hashmap_bytes) {
        *hashmap_bytes = nmemb * (sizeof(uint32_t));
    }

    if (cache_bytes) {
        *cache_bytes = nmemb * (sizeof(cm_entry) + size_a);
    }

    return 0;
}

cm_entry *
cm_entry_ptr(cm_cache *cm, uint32_t slot)
{
    uintptr_t cache = (uintptr_t)cm->cache;
    size_t offset = slot * (sizeof(cm_entry) + cm->size);
    return (slot != CM_NIL) ? (cm_entry *)(cache + offset) : NULL;
}

void
cm_move_chain(cm_cache *cm, uint32_t slot, uint32_t old_hash, uint32_t new_hash)
{
    assert(old_hash != CM_NIL);

    uint32_t *index;
    cm_entry *e = cm_entry_ptr(cm, slot);

    if (new_hash == CM_NIL) {
        index = &slot;
    } else if (cm->hashmap[new_hash] != slot) {
        index = &cm->hashmap[new_hash];
    } else {
        assert(old_hash == new_hash);
        return;
    }

    if (e->clru != CM_NIL) {
        cm_entry_ptr(cm, e->clru)->cmru = e->cmru;
    } else {
        // no lru pointer in collision chain
    }

    if (e->cmru != CM_NIL) {
        cm_entry_ptr(cm, e->cmru)->clru = e->clru;
    } else if (e->clru != slot) {
        cm->hashmap[old_hash] = e->clru;
    }

    // If removal requested, "*index == i" => no longer in any chain.
    e->clru = *index;

    // If relocation requested, "s->hashmap[new_hash]->cmru = i" => first element in new chain.
    if (e->clru != CM_NIL) {
        cm_entry_ptr(cm, e->clru)->cmru = slot;
    }

    // Element is the most used in both cases
    e->cmru = CM_NIL;

    // If relocation requested: "s->hashmap[new_hash] = i" => first element in new chain.
    // If removal requested: "i = i" => NOP
    *index = slot;

    check_invariants(cm);
}

void
cm_make_lru(cm_cache *cm, uint32_t slot)
{
    if (cm->lru == slot) return;

    cm_entry *e = cm_entry_ptr(cm, slot);

    unlink(cm, e);

    e->mru = cm->lru;
    cm_entry_ptr(cm, e->mru)->lru = slot;
    cm->lru = slot;

    assert((e->lru == CM_NIL) || (cm_entry_ptr(cm, e->lru)->mru == slot));
    assert((e->mru == CM_NIL) || (cm_entry_ptr(cm, e->mru)->lru == slot));

    assert(cm->mru != CM_NIL);
    assert(cm->lru == slot);
}

void
cm_make_mru(cm_cache *cm, uint32_t slot)
{
    if (cm->mru == slot) return;

    cm_entry *e = cm_entry_ptr(cm, slot);

    unlink(cm, e);

    e->lru = cm->mru;
    cm_entry_ptr(cm, e->lru)->mru = slot;

    cm->mru = slot;

    assert((e->lru == CM_NIL) || (cm_entry_ptr(cm, e->lru)->mru == slot));
    assert((e->mru == CM_NIL) || (cm_entry_ptr(cm, e->mru)->lru == slot));

    assert(cm->lru != CM_NIL);
    assert(cm->mru == slot);
}

int
cm_init(cm_cache *s, uint32_t size_a, cm_hash *hash, cm_compare *compare, cm_destroy *destroy)
{
    if (size_a == 0) {
        return EINVAL;
    }

    if (compare == NULL || hash == NULL) {
        return EINVAL;
    }

    s->hashmap = NULL;
    s->cache = NULL;

    s->psel_bit = 0;
    s->psel_ctr = 0;
    s->leader_set_size = 0;

    s->destroy = destroy;
    s->compare = compare;
    s->hash = hash;

    s->size = size_a;
    s->nmemb = 0;

    s->lru = CM_NIL;
    s->mru = CM_NIL;
    return 0;
}

bool
cm_is_full(cm_cache *s)
{
    cm_entry *entry = cm_entry_ptr(s, s->lru);
    return (entry == NULL) || (entry->clru != s->lru);
}

int
cm_set_size(cm_cache *cm, uint32_t nmemb, size_t *hashmap_bytes, size_t *cache_bytes)
{
    int rv = 0;
    uint32_t i;
    uint32_t old_hash;
    uint32_t new_hash;
    cm_entry *e;

    rv = cm_required_bytes(cm->size, nmemb, hashmap_bytes, cache_bytes);
    if (rv != 0) {
        return rv;
    }

    if (nmemb < cm->nmemb) {
        for (i = nmemb; i < cm->nmemb; i++) {
            e = cm_entry_ptr(cm, i);
            old_hash = cm->hash(e->key, cm->nmemb);

            if (e->clru != i && cm->destroy) {
                cm->destroy(e->key, i);
            }

            cm_move_chain(cm, i, old_hash, CM_NIL);
            unlink(cm, e);
        }

        assert(cm->lru < nmemb);
        assert(cm->mru < nmemb);

        for (i = cm->mru; (e = cm_entry_ptr(cm, i)) && e->clru != i; i = e->lru) {
            assert(i < cm->nmemb);

            old_hash = cm->hash(e->key, cm->nmemb);
            new_hash = cm->hash(e->key, nmemb);
            cm_move_chain(cm, i, old_hash, new_hash);
        }

        cm->nmemb = nmemb;
    }

    cm->try_nmemb = nmemb;
    return rv;
}

int
cm_set_data(cm_cache *s, void *hashmap, void *cache)
{
    uint32_t i;
    uint32_t old_hash;
    uint32_t new_hash;
    cm_entry *e;

    size_t hashmap_bytes = s->try_nmemb * sizeof(*s->hashmap);
    size_t cache_bytes = s->try_nmemb * (sizeof(cm_entry) + s->size);

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
            e = cm_entry_ptr(s, i);

            e->lru = (i > s->nmemb) ? (i - 1) : CM_NIL;
            e->mru = (i < s->try_nmemb - 1) ? (i + 1) : s->lru;

            /*
             * The member clru of a cache entry is tri-state:
             *
             * 1. Not in a collision chain (e->clru == i)
             * 2. The last element of a collision chain (e->clru == CM_NIL)
             * 3. Any other element of a collision chain (e->clru != i && e->clru != lru_cache_entry_nil)
             */
            e->clru = i;
            e->cmru = CM_NIL;

            s->hashmap[i] = CM_NIL;
        }

        if (s->lru != CM_NIL) {
            cm_entry_ptr(s, s->lru)->lru = s->try_nmemb - 1;
        }

        s->lru = s->nmemb;

        if (s->mru == CM_NIL) {
            s->mru = s->try_nmemb - 1;
        }

        for (i = s->mru; (e = cm_entry_ptr(s, i)) && e->clru != i; i = e->lru) {
            assert(i < s->nmemb && i < s->try_nmemb);

            old_hash = s->hash(e->key, s->nmemb);
            new_hash = s->hash(e->key, s->try_nmemb);
            cm_move_chain(s, i, old_hash, new_hash);
        }

        s->nmemb = s->try_nmemb;
    }

    // guaranteed by correct order of set_nmemb and set_memory
    assert(s->nmemb == s->try_nmemb);
    return 0;
}

uint32_t
cm_put_key(cm_cache *s, const void *key)
{
    // 11. Cache miss -- determine insertion mode
    uint32_t i = s->lru;
    cm_entry *e = cm_entry_ptr(s, i);
    uint32_t new_hash = s->hash(key, s->nmemb);
    uint32_t old_hash = new_hash;

    if (e->clru != i) {
        old_hash = s->hash(e->key, s->nmemb);

        if (s->destroy) {
            s->destroy(e->key, i);
        }
    }

    memcpy(e->key, key, s->size);
    cm_make_mru(s, i);
    cm_move_chain(s, i, old_hash, new_hash);
    return i;
}

uint32_t
cm_get_or_put_key(cm_cache *s, const void *key, bool *put)
{
    if (s->nmemb == 0) {
        return CM_NIL;
    }

    // 3. Extract Components
    uint32_t new_hash = s->hash(key, s->nmemb);
    // uint32_t old_hash = new_hash;
    uint32_t i = s->hashmap[new_hash];
    cm_entry *e = NULL;

    // 4. Check for cache hit
    while ((e = cm_entry_ptr(s, i))) {
        if (s->compare(e->key, key) == 0) {
            if (put) {
                *put = false;
            }

            // 8. Protomote to LRU
            cm_make_mru(s, i);
            cm_move_chain(s, i, new_hash, new_hash);
            return i;
        }

        assert(e->clru != i);
        i = e->clru;
    }

    if (!put) {
        return CM_NIL;
    }

    *put = true;
    return cm_put_key(s, key);
}

void
cm_flush(cm_cache *s)
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
    cm_entry *e;

    for (i = s->mru; (e = cm_entry_ptr(s, i)) && e->clru != i; i = e->lru) {
        old_hash = s->hash(e->key, s->nmemb);

        if (s->destroy) {
            s->destroy(e->key, i);
        }

        cm_move_chain(s, i, old_hash, CM_NIL);
    }
}
