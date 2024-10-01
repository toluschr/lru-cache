#include "lru-cache.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <memory.h>

#define CACHE_ENTRY_NIL UINT32_MAX

uint32_t lru_cache_get_entry_offset(uint32_t i, uint32_t s)
{
    return i * (sizeof(struct lru_cache_entry) + s);
}

struct lru_cache_entry *get_entry(struct lru_cache *s, uint32_t i)
{
    if (i == CACHE_ENTRY_NIL) {
        return NULL;
    }

    char *cache = s->cache;
    uint32_t offset = lru_cache_get_entry_offset(i, s->size);

    return (struct lru_cache_entry *)(cache + offset);
}

void lru_cache_print(struct lru_cache *s, FILE *file)
{
    bool visited[s->nmemb];
    memset(visited, 0, sizeof(visited));

    for (int i = 0; i < s->nmemb; i++) {
        printf("[%d]", i);

        uint32_t entry_id = s->hashmap[i];
        struct lru_cache_entry *entry = NULL;

        while (entry_id != CACHE_ENTRY_NIL) {
            entry = get_entry(s, entry_id);

            printf(" --> %d", entry_id);

            entry_id = entry->clru;
        }

        printf("\n");
    }
}

bool lru_cache_get_or_put(struct lru_cache *s, const void *key, uint32_t *index)
{
    bool found = false;
    uint32_t hash = s->hash(key, s->size);
    uint32_t old_hash = hash;

    uint32_t i = s->hashmap[hash % s->nmemb];

    struct lru_cache_entry *e = NULL;

    while (i != CACHE_ENTRY_NIL) {
        e = get_entry(s, i);

        if (s->compare(e->key, key, s->size) == 0) {
            found = true;
            goto access_and_rearrange;
        }

        i = e->clru;
    }

    // remove from collision chain
    i = s->lru;
    e = get_entry(s, i);

    old_hash = s->hash(e->key, s->size);

    if (s->destroy)
        s->destroy(e->key, s->size);

    memcpy(e->key, key, s->size);

access_and_rearrange:
    // Make current entry most recently used in the global chain if not already
    if (s->mru != i) {
        if (e->lru != CACHE_ENTRY_NIL) {
            get_entry(s, e->lru)->mru = e->mru;
        }

        if (e->mru != CACHE_ENTRY_NIL) {
            get_entry(s, e->mru)->lru = e->lru;
        }

        get_entry(s, s->mru)->mru = i;

        e->lru = s->mru;
        s->mru = i;

        s->lru = e->mru;
        e->mru = CACHE_ENTRY_NIL;

        assert((e->lru == CACHE_ENTRY_NIL) || (get_entry(s, e->lru)->mru == i));
        assert((e->mru == CACHE_ENTRY_NIL) || (get_entry(s, e->mru)->lru == i));

        assert(s->lru != CACHE_ENTRY_NIL);
        assert(s->mru == i);
    }

    // Make current entry most recently used in the local chain if not already
    if (s->hashmap[hash % s->nmemb] != i) {
        if (e->clru != CACHE_ENTRY_NIL) {
            get_entry(s, e->clru)->cmru = e->cmru;
        }

        if (e->cmru != CACHE_ENTRY_NIL) {
            get_entry(s, e->cmru)->clru = e->clru;
        } else if (e->clru != i) {
            s->hashmap[old_hash % s->nmemb] = e->clru;
        }

        e->clru = s->hashmap[hash % s->nmemb];
        if (e->clru != CACHE_ENTRY_NIL) {
            get_entry(s, e->clru)->cmru = i;
        }

        s->hashmap[hash % s->nmemb] = i;
    }

    if (index) {
        *index = i;
    }

    return found;
}

uint32_t my_hash(const void *a_, size_t l)
{
    const char *a = a_;
    uint32_t out = 2147483647;

    if (a[0] == 1) return 0;

    while (l--) {
        out ^= *a++;
        out = (out >> 1) | (out << 31);
    }

    return out;
}

void lru_cache_flush(struct lru_cache *s)
{
}

void lru_cache_init_in_memory(struct lru_cache *s, size_t size, size_t nmemb, void *hashmap, void *cache, lru_cache_destroy_t destroy, lru_cache_compare_t compare, lru_cache_hash_t hash)
{
    s->nmemb = nmemb;
    s->size = size;

    s->hashmap = hashmap;
    s->cache = cache;

    s->destroy = destroy;
    s->compare = compare;
    s->hash = hash;

    s->lru = 0;
    s->mru = s->nmemb - 1;

    for (uint32_t i = 0; i < s->nmemb; i++) {
        struct lru_cache_entry *cur = get_entry(s, i);

        cur->lru = (i > 0) ? (i - 1) : CACHE_ENTRY_NIL;
        cur->mru = (i < s->nmemb - 1) ? (i + 1) : CACHE_ENTRY_NIL;

        cur->clru = i;
        cur->cmru = CACHE_ENTRY_NIL;

        s->hashmap[i] = CACHE_ENTRY_NIL;
    }
}

/*
int main()
{
    struct lru_cache c;
    c.nmemb = 2;
    c.size = sizeof(char);

    // lru_cache_init_in_memory();

    uint32_t cache_size = c.nmemb * (sizeof(struct lru_cache_entry) + c.size);
    uint32_t hashmap_size = c.nmemb * sizeof(uint32_t);

    c.cache = malloc(cache_size);
    c.hashmap = malloc(hashmap_size);

    c.destroy = NULL;
    c.compare = memcmp;
    c.hash = my_hash;

    for (uint32_t i = 0; i < c.nmemb; i++) {
        struct lru_cache_entry *cur = get_entry(&c, i);

        cur->lru = (i > 0) ? (i - 1) : CACHE_ENTRY_NIL;
        cur->mru = (i < c.nmemb - 1) ? (i + 1) : CACHE_ENTRY_NIL;

        cur->clru = i;
        cur->cmru = CACHE_ENTRY_NIL;

        c.hashmap[i] = CACHE_ENTRY_NIL;
    }

    c.lru = 0;
    c.mru = c.nmemb - 1;

    printf("%d\n", lru_cache_get_or_put(&c, "\0", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\1", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\1", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\1", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\0", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\1", NULL));
    printf("%d\n", lru_cache_get_or_put(&c, "\0", NULL));
    // printf("%d\n", lru_cache_get_or_put(&c, "\0", NULL));
    // printf("%d\n", lru_cache_get_or_put(&c, "\0", NULL));

    // lru_cache_get_or_put(&c, "b", NULL);
    // lru_cache_get_or_put(&c, "c", NULL);
    // lru_cache_get_or_put(&c, "d", NULL);
    // lru_cache_get_or_put(&c, "e", NULL);
    // lru_cache_get_or_put(&c, "f", NULL);
    // lru_cache_get_or_put(&c, "g", NULL);
    // lru_cache_get_or_put(&c, "h", NULL);
    // lru_cache_get_or_put(&c, "i", NULL);
    // lru_cache_get_or_put(&c, "j", NULL);
    // lru_cache_get_or_put(&c, "k", NULL);
    // lru_cache_get_or_put(&c, "l", NULL);
    // lru_cache_get_or_put(&c, "m", NULL);
    // lru_cache_get_or_put(&c, "n", NULL);
    // lru_cache_get_or_put(&c, "o", NULL);
    // lru_cache_get_or_put(&c, "p", NULL);
    // lru_cache_get_or_put(&c, "q", NULL);
    // lru_cache_get_or_put(&c, "r", NULL);
    // lru_cache_get_or_put(&c, "s", NULL);
    // lru_cache_get_or_put(&c, "t", NULL);
    // lru_cache_get_or_put(&c, "u", NULL);
    // lru_cache_get_or_put(&c, "v", NULL);
    // lru_cache_get_or_put(&c, "w", NULL);
    // lru_cache_get_or_put(&c, "x", NULL);
    // lru_cache_get_or_put(&c, "y", NULL);
    // lru_cache_get_or_put(&c, "z", NULL);

    // printf("%d, %d\n", cache_size, hashmap_size);
}
*/

