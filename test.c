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
static cm_cache c;

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

    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 2, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) == 0 && put);
    assert(cm_get_or_put_key(&c, "b", &put) == 1 && put);

    free(hashmap);
    free(cache);
}

static void test_cache_collision_first_in_local_chain(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 2, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    // a and b have the same hash and can both be uniquely inserted
    assert(!cm_is_full(&c));
    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);

    assert(!cm_is_full(&c));
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);

    assert(cm_is_full(&c));

    // a and b should not evict each other from the cache
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);

    assert(cm_is_full(&c));

    eviction = "a";
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(*eviction == 0);

    assert(cm_is_full(&c));

    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_full_no_collisions(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "e", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "f", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "h", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "i", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "j", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "k", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "l", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "m", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "n", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "o", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "p", &put) != CM_NIL && put);
    assert(cm_is_full(&c));

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "e", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "f", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "g", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "h", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "i", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "j", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "k", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "l", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "m", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "n", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "o", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "p", NULL) != CM_NIL);
    assert(cm_is_full(&c));

    free(hashmap);
    free(cache);
}

static void test_cache_invalid_alignment(void)
{
    assert(cm_align(sizeof(char), 0, NULL) == EOVERFLOW);
    assert(cm_align(sizeof(char), sizeof(cm_entry) + 1, NULL) == EINVAL);
    assert(cm_align(sizeof(char), sizeof(cm_entry), NULL) == 0);
}

static void test_cache_invalid_size_nmemb(void)
{
    cm_cache c;
    assert(cm_init(&c, 0, hash_to_zero, my_compare, NULL) == EINVAL);
    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, NULL) == 0);

    assert(cm_set_size(&c, 0, NULL, NULL) == EINVAL);
    assert(cm_set_size(&c, 1, NULL, NULL) == 0);
}

static void test_cache_single_entry(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_is_full(&c));
    assert(cm_set_data(&c, hashmap, cache) == 0);
    assert(!cm_is_full(&c));

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_is_full(&c));

    eviction = "a";
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(*eviction == 0);

    eviction = "b";
    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(*eviction == 0);

    eviction = "a";
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(*eviction == 0);

    eviction = "";
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);

    eviction = "b";
    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(*eviction == 0);

    eviction = "";
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_is_full(&c));

    free(hashmap);
    free(cache);
}

static void test_cache_random_access(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "e", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "f", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "e", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "h", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "i", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "j", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "k", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "k", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "l", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "m", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "l", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "n", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "o", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "p", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "e", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "f", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "g", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "h", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "i", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "j", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "k", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "l", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "m", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "n", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "o", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "p", NULL) != CM_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_shrink(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "e", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "f", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "h", &put) != CM_NIL && put);

    eviction = "efgh";
    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(*eviction == 0);

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    eviction = "";
    assert(cm_set_data(&c, hashmap, cache) == 0);
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);

    eviction = "abcd";
    assert(cm_get_or_put_key(&c, "e", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "f", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "h", &put) != CM_NIL && put);
    assert(*eviction == 0);

    eviction = "efgh";
    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);
    assert(*eviction == 0);

    free(hashmap);
    free(cache);
}

static void test_cache_grow_full(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);

    assert(cm_is_full(&c));
    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_is_full(&c));

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(!cm_is_full(&c));
    assert(cm_get_or_put_key(&c, "e", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "f", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "g", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "h", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);

    assert(cm_get_or_put_key(&c, "e", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "f", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "g", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "h", NULL) != CM_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_simple_flush(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_self, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 16, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);

    eviction = "ba";
    cm_flush(&c);
    assert(*eviction == 0);

    eviction = "";
    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);

    free(hashmap);
    free(cache);
}

static void test_cache_flush_full_with_collisions(void)
{
    bool put;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    eviction = "";
    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);

    eviction = "dcba";
    cm_flush(&c);
    assert(*eviction == 0);

    assert(cm_get_or_put_key(&c, "a", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "b", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "c", &put) != CM_NIL && put);
    assert(cm_get_or_put_key(&c, "d", &put) != CM_NIL && put);

    assert(cm_get_or_put_key(&c, "a", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "b", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "c", NULL) != CM_NIL);
    assert(cm_get_or_put_key(&c, "d", NULL) != CM_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_set_size_initial_multi(void)
{
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 2, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 1, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);
    free(hashmap);
    free(cache);
}

static void test_cache_set_size_multi(void)
{
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(cm_init(&c, sizeof(char), hash_to_zero, my_compare, destroy) == 0);

    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);

    assert(cm_set_size(&c, 16, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 7, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 8, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 4, &hashmap_bytes, &cache_bytes) == 0);
    assert(cm_set_size(&c, 5, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = realloc(hashmap, hashmap_bytes);
    cache = realloc(cache, cache_bytes);

    assert(cm_set_data(&c, hashmap, cache) == 0);
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
    TEST(test_cache_set_size_initial_multi);
    TEST(test_cache_set_size_multi);
    TEST(test_cache_insert_order);
}
