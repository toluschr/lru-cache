#include "cachemap.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <memory.h>
#include <errno.h>

// select a deterministic pseudo random number (need to know the last insertion policy on eviction)
static void
update_hashes(cm_cache *cm)
{
    uint32_t i;
    cm_entry *e;

    CM_ITER_VALID_ENTRIES(cm, i, e) {
        uint32_t old_hash = cm->hash(e->key, cm->nmemb);
        uint32_t new_hash = cm->hash(e->key, cm->try_nmemb);
        cm_move_chain(cm, i, old_hash, new_hash);
    }

    CM_ITER_PINNED_ENTRIES(cm, i, e) {
        uint32_t old_hash = cm->hash(e->key, cm->nmemb);
        uint32_t new_hash = cm->hash(e->key, cm->try_nmemb);
        cm_move_chain(cm, i, old_hash, new_hash);
    }

    cm->nmemb = cm->try_nmemb;
}

static void
unlink(cm_cache *cm, uint32_t slot)
{
    cm_entry *e = cm_entry_ptr(cm, slot);

    if (cm->lru != slot) {
        cm_entry_ptr(cm, e->lru)->mru = e->mru;
    } else {
        cm->lru = cm->mru != slot ? e->mru : CM_NIL;
    }

    if (cm->mru != slot) {
        cm_entry_ptr(cm, e->mru)->lru = e->lru;
    } else {
        cm->mru = e->lru != CM_NIL ? e->lru : e->mru;
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

    uint32_t *new_head;
    cm_entry *e = cm_entry_ptr(cm, slot);

    if (new_hash == CM_NIL && e->clru != slot && cm->destroy) {
        cm->destroy(e->key, slot);
    }

    if (new_hash == CM_NIL) {
        new_head = &slot;
    } else if (cm->hashmap[new_hash] != slot) {
        new_head = &cm->hashmap[new_hash];
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

    // If removal requested, "*new_head == i" => no longer in any chain.
    e->clru = *new_head;

    // If relocation requested, "s->hashmap[new_hash]->cmru = i" => first element in new chain.
    if (e->clru != CM_NIL) {
        cm_entry_ptr(cm, e->clru)->cmru = slot;
    }

    // Element is the most used in both cases
    e->cmru = CM_NIL;

    // If relocation requested: "s->hashmap[new_hash] = i" => first element in new chain.
    // If removal requested: "i = i" => NOP
    *new_head = slot;

    assert(e->clru != slot || new_hash == CM_NIL);
}

void
cm_make_pin(cm_cache *cm, uint32_t slot)
{
    assert(cm->lru != CM_NIL || cm->mru != CM_NIL);
    cm_entry *e = cm_entry_ptr(cm, slot);

    cm_make_mru(cm, slot);

    if (e->lru == CM_NIL) {
        cm->lru = CM_NIL;
    } else {
        cm->mru = e->lru;
    }
}

void
cm_make_lru(cm_cache *cm, uint32_t slot)
{
    if (cm->lru == slot) return;

    assert(cm->lru != CM_NIL || cm->mru != CM_NIL);
    cm_entry *e = cm_entry_ptr(cm, slot);

    unlink(cm, slot);

    if (cm->lru == CM_NIL) {
        cm->mru = slot;
    }

    e->mru = (cm->lru == CM_NIL) ? cm->mru : cm->lru;
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
    if (cm->mru == slot && cm->lru == CM_NIL) cm->lru = slot;
    if (cm->mru == slot) return;

    assert(cm->lru != CM_NIL || cm->mru != CM_NIL);
    cm_entry *e = cm_entry_ptr(cm, slot);
    unlink(cm, slot);

    cm_entry *mru = cm_entry_ptr(cm, cm->mru);
    e->mru = mru->mru;

    if (cm->lru != CM_NIL) {
        mru->mru = slot;
        e->lru = cm->mru;
    } else {
        mru->lru = slot;
        cm->lru = slot;
    }

    cm->mru = slot;
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

    rv = cm_required_bytes(cm->size, nmemb, hashmap_bytes, cache_bytes);
    if (rv != 0) return rv;

    cm->try_nmemb = nmemb;

    if (cm->try_nmemb < cm->nmemb) {
        for (uint32_t i = cm->try_nmemb; i < cm->nmemb; i++) {
            cm_entry *e = cm_entry_ptr(cm, i);
            uint32_t old_hash = cm->hash(e->key, cm->nmemb);

            cm_move_chain(cm, i, old_hash, CM_NIL);
            unlink(cm, i);
        }

        assert(cm->lru == CM_NIL || cm->lru < cm->try_nmemb);
        assert(cm->mru == CM_NIL || cm->mru < cm->try_nmemb);
        update_hashes(cm);
    }

    return rv;
}

int
cm_set_data(cm_cache *cm, void *hashmap, void *cache)
{
    int rv;
    size_t hashmap_bytes, cache_bytes;

    rv = cm_required_bytes(cm->size, cm->try_nmemb, &hashmap_bytes, &cache_bytes);
    if (rv != 0) return rv;

    if (UINTPTR_MAX - (uintptr_t)cache < cache_bytes) {
        return EOVERFLOW;
    }

    if (UINTPTR_MAX - (uintptr_t)hashmap < hashmap_bytes) {
        return EOVERFLOW;
    }

    cm->hashmap = hashmap;
    cm->cache = cache;

    if (cm->nmemb < cm->try_nmemb) {
        for (uint32_t i = cm->nmemb; i < cm->try_nmemb; i++) {
            cm_entry *e = cm_entry_ptr(cm, i);

            e->lru = (i > cm->nmemb) ? (i - 1) : CM_NIL;
            e->mru = (i < cm->try_nmemb - 1) ? (i + 1) : cm->lru;

            e->clru = i;
            e->cmru = CM_NIL;

            cm->hashmap[i] = CM_NIL;
        }

        if (cm->mru != CM_NIL && cm->lru == CM_NIL) {
            cm_entry_ptr(cm, cm->mru)->lru = cm->try_nmemb - 1;
            cm_entry_ptr(cm, cm->try_nmemb - 1)->mru = cm->mru;
        }

        if (cm->mru == CM_NIL || cm->lru == CM_NIL) {
            cm->mru = cm->try_nmemb - 1;
        }

        cm->lru = cm->nmemb;
        update_hashes(cm);
    }

    // guaranteed by correct order of set_nmemb and set_memory
    assert(cm->nmemb == cm->try_nmemb);
    return 0;
}

uint32_t
cm_put_key(cm_cache *s, const void *key)
{
    // The cache is uninitialized or all members are pinned
    if (s->lru == CM_NIL) {
        return CM_NIL;
    }

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

    uint32_t i = s->hashmap[s->hash(key, s->nmemb)];
    cm_entry *e = NULL;

    while ((e = cm_entry_ptr(s, i))) {
        if (s->compare(e->key, key) == 0) {
            if (put) {
                *put = false;
            }

            cm_make_mru(s, i);
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
cm_flush(cm_cache *cm)
{
    uint32_t i;
    cm_entry *e;

    CM_ITER_VALID_ENTRIES(cm, i, e) {
        cm_move_chain(cm, i, cm->hash(e->key, cm->nmemb), CM_NIL);
    }
}
