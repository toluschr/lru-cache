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
    struct lru_cache c;
    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init_size(&c, sizeof(char), 2, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_init_memory(&c, hashmap, cache, hash_to_zero, my_compare, NULL) == 0);

    // a and b have the same hash and can both be uniquely inserted
    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 0);

    // a and b should not evict each other from the cache
    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 1);

    assert(lru_cache_get_or_put(&c, "c", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "c", NULL) == 1);

    free(hashmap);
    free(cache);
}

/*
static void test_cache_lru_mru(void)
{
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init_size(&c, sizeof(char), 16, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_init_memory(&c, hashmap, cache, hash_to_self, my_compare, NULL) == 0);
}
*/

static void test_cache_full_no_collisions(void)
{
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init_size(&c, sizeof(char), 16, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_init_memory(&c, hashmap, cache, hash_to_self, my_compare, NULL) == 0);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "c", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "d", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "e", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "f", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "h", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "i", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "j", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "k", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "l", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "m", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "n", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "o", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "p", NULL) == 0);

    // lru_cache_print(&c, stdout);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "c", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "d", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "e", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "f", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "h", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "i", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "j", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "k", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "l", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "m", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "n", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "o", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "p", NULL) == 1);

    free(hashmap);
    free(cache);
}

static void test_cache_invalid_alignment(void)
{
    struct lru_cache c;
    assert(lru_cache_init_size(&c, sizeof(char), 1, 0, NULL, NULL) == EINVAL);
    assert(lru_cache_init_size(&c, sizeof(char), 1, sizeof(struct lru_cache_entry) + 1, NULL, NULL) == EINVAL);
    assert(lru_cache_init_size(&c, sizeof(char), 1, sizeof(struct lru_cache_entry), NULL, NULL) == 0);
}

static void test_cache_invalid_size_nmemb(void)
{
    struct lru_cache c;
    assert(lru_cache_init_size(&c, 0, 1, 1, NULL, NULL) == EINVAL);
    assert(lru_cache_init_size(&c, 1, 0, 1, NULL, NULL) == EINVAL);
}

static void test_cache_single_entry(void)
{
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init_size(&c, sizeof(char), 1, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_init_memory(&c, hashmap, cache, hash_to_self, my_compare, NULL) == 0);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);

    assert(lru_cache_get_or_put(&c, "b", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 0);

    assert(lru_cache_get_or_put(&c, "b", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 1);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);

    free(hashmap);
    free(cache);
}

static void test_cache_random_access(void)
{
    struct lru_cache c;

    size_t hashmap_bytes, cache_bytes;
    void *hashmap, *cache;

    assert(lru_cache_init_size(&c, sizeof(char), 16, 1, &hashmap_bytes, &cache_bytes) == 0);

    hashmap = malloc(hashmap_bytes);
    cache = malloc(cache_bytes);

    assert(lru_cache_init_memory(&c, hashmap, cache, hash_to_self, my_compare, NULL) == 0);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "c", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "d", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "e", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "f", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "e", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "h", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "d", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "i", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "j", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "k", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "k", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "l", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "m", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "l", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "n", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "o", NULL) == 0);
    assert(lru_cache_get_or_put(&c, "p", NULL) == 0);

    assert(lru_cache_get_or_put(&c, "a", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "b", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "c", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "d", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "e", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "f", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "g", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "h", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "i", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "j", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "k", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "l", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "m", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "n", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "o", NULL) == 1);
    assert(lru_cache_get_or_put(&c, "p", NULL) == 1);

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
