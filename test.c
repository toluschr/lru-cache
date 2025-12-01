#include "lru-cache.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define TEST(NAME) \
    { \
        fprintf(stderr, "%s\n", #NAME); \
        NAME(); \
        fprintf(stderr, "\r\033[A%s \033[32;1mOK\033[0m\n", #NAME); \
    }

static const char *eviction = "";
static struct lru_cache c;

static void destroy(void *key, uint32_t idx)
{
    (void)idx;

    assert(*(char *)key == *eviction++);
}

static uint32_t hash_to_zero(const void *a_, uint32_t m_)
{
    return (void)a_, (void)m_, 0u;
}

static uint32_t hash_to_self(const void *a_, uint32_t m_)
{
    return (*(char *)a_ - 'a') % m_;
}

static int my_compare(const void *a_, const void *b_)
{
    const char *a = a_;
    const char *b = b_;

    return (*a - *b);
}

static void test_cache_insert_order(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 2, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) == 0 && put);
    assert(lru_cache_get_or_put(&c, "b", &put) == 1 && put);

    free(hashmap);
    free(cache);
}

static void test_cache_collision_first_in_local_chain(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 2, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    // a and b have the same hash and can both be uniquely inserted
    assert(!lru_cache_is_full(&c));
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(!lru_cache_is_full(&c));
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_is_full(&c));

    // a and b should not evict each other from the cache
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_is_full(&c));

    eviction = "a";
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    assert(lru_cache_is_full(&c));

    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_full_no_collisions(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "f", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "i", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "j", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "k", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "l", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "m", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "n", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "o", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "p", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_is_full(&c));

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "e", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "f", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "g", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "h", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "i", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "j", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "k", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "l", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "m", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "n", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "o", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "p", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_is_full(&c));

    free(hashmap);
    free(cache);
}

static void test_cache_invalid_alignment(void)
{
    assert(lru_cache_align(sizeof(char), 0, NULL) == EOVERFLOW);
    assert(lru_cache_align(sizeof(char), sizeof(struct lru_cache_entry) + 1, NULL) == EINVAL);
    assert(lru_cache_align(sizeof(char), sizeof(struct lru_cache_entry), NULL) == 0);
}

static void test_cache_invalid_size_nmemb(void)
{
    struct lru_cache c;
    assert(lru_cache_init(&c, 0, hash_to_zero, my_compare, NULL) == EINVAL);
    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, NULL) == 0);

    assert(lru_cache_set_nmemb(&c, 0, NULL, NULL) == EINVAL);
    assert(lru_cache_set_nmemb(&c, 1, NULL, NULL) == 0);
}

static void test_cache_single_entry(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_is_full(&c));
    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);
    assert(!lru_cache_is_full(&c));

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_is_full(&c));

    eviction = "a";
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    eviction = "b";
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    eviction = "a";
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    eviction = "";
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);

    eviction = "b";
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    eviction = "";
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_is_full(&c));

    free(hashmap);
    free(cache);
}

static void test_cache_random_access(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "f", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "e", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "i", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "j", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "k", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "k", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "l", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "m", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "l", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "n", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "o", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "p", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "e", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "f", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "g", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "h", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "i", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "j", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "k", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "l", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "m", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "n", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "o", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "p", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_shrink(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "f", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);

    eviction = "efgh";
    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(*eviction == 0);

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    eviction = "";
    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);

    eviction = "abcd";
    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "f", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    eviction = "efgh";
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(*eviction == 0);

    free(hashmap);
    free(cache);
}

static void test_cache_grow_full(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_is_full(&c));
    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_is_full(&c));

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(!lru_cache_is_full(&c));
    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "f", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_get_or_put(&c, "e", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "f", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "g", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "h", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_simple_flush(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);

    eviction = "ba";
    lru_cache_flush(&c);
    assert(*eviction == 0);

    eviction = "";
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);

    free(hashmap);
    free(cache);
}

static void test_cache_flush_full_with_collisions(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);

    eviction = "dcba";
    lru_cache_flush(&c);
    assert(*eviction == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "d", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_set_nmemb_initial_multi(void)
{
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 2, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 1, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    free(hashmap);
    free(cache);
}

static void test_cache_set_nmemb_multi(void)
{
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_set_nmemb(&c, 16, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 7, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(lru_cache_set_nmemb(&c, 5, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    free(hashmap);
    free(cache);
}

int main()
{
    TEST(test_cache_collision_first_in_local_chain);
    TEST(test_cache_invalid_alignment);
    TEST(test_cache_invalid_size_nmemb);
    TEST(test_cache_full_no_collisions);
    TEST(test_cache_single_entry);
    TEST(test_cache_random_access);
    TEST(test_cache_shrink);
    TEST(test_cache_grow_full);
    TEST(test_cache_simple_flush);
    TEST(test_cache_flush_full_with_collisions);
    TEST(test_cache_set_nmemb_initial_multi);
    TEST(test_cache_set_nmemb_multi);
    TEST(test_cache_insert_order);
}
