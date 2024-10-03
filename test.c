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

uint32_t hash_to_zero(const void *a_)
{
    return (void)a_, 0u;
}

uint32_t hash_to_self(const void *a_)
{
    return *(char *)a_;
}

int my_compare(const void *a_, const void *b_)
{
    const char *a = a_;
    const char *b = b_;

    return (*a - *b);
}

static void test_cache_collision_first_in_local_chain(void)
{
    bool put;
    struct lru_cache c;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), 1, hash_to_zero, my_compare, NULL) == 0);

    assert(lru_cache_set_nmemb(&c, 2, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    // a and b have the same hash and can both be uniquely inserted
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);

    // a and b should not evict each other from the cache
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_get_or_put(&c, "c", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "c", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_full_no_collisions(void)
{
    bool put;
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), 1, hash_to_self, my_compare, NULL) == 0);

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

    // lru_cache_print(&c, stdout);

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

static void test_cache_invalid_alignment(void)
{
    struct lru_cache c;
    assert(lru_cache_init(&c, sizeof(char), 0, hash_to_zero, my_compare, NULL) == EINVAL);
    assert(lru_cache_init(&c, sizeof(char), sizeof(struct lru_cache_entry) + 1, hash_to_zero, my_compare, NULL) == EINVAL);
    assert(lru_cache_init(&c, sizeof(char), sizeof(struct lru_cache_entry), hash_to_zero, my_compare, NULL) == 0);
}

static void test_cache_invalid_size_nmemb(void)
{
    struct lru_cache c;
    assert(lru_cache_init(&c, 0, 1, hash_to_zero, my_compare, NULL) == EINVAL);
    assert(lru_cache_init(&c, sizeof(char), 1, hash_to_zero, my_compare, NULL) == 0);

    assert(lru_cache_set_nmemb(&c, 0, NULL, NULL) == EINVAL);
    assert(lru_cache_set_nmemb(&c, 1, NULL, NULL) == 0);
}

static void test_cache_single_entry(void)
{
    bool put;
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), 1, hash_to_self, my_compare, NULL) == 0);

    assert(lru_cache_set_nmemb(&c, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_set_memory(&c, hashmap, cache) == 0);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "b", &put) != LRU_CACHE_ENTRY_NIL && put);

    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "b", NULL) != LRU_CACHE_ENTRY_NIL);

    assert(lru_cache_get_or_put(&c, "a", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "a", NULL) != LRU_CACHE_ENTRY_NIL);

    free(hashmap);
    free(cache);
}

static void test_cache_random_access(void)
{
    bool put;
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init(&c, sizeof(char), 1, hash_to_self, my_compare, NULL) == 0);
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
    assert(lru_cache_get_or_put(&c, "e", &put) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "h", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "d", &put) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "i", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "j", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "g", &put) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "k", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "k", &put) != LRU_CACHE_ENTRY_NIL);
    assert(lru_cache_get_or_put(&c, "l", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "m", &put) != LRU_CACHE_ENTRY_NIL && put);
    assert(lru_cache_get_or_put(&c, "l", &put) != LRU_CACHE_ENTRY_NIL);
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

int main()
{
    TEST(test_cache_collision_first_in_local_chain);
    TEST(test_cache_invalid_alignment);
    TEST(test_cache_invalid_size_nmemb);
    TEST(test_cache_full_no_collisions);
    TEST(test_cache_single_entry);
    TEST(test_cache_random_access);
}
