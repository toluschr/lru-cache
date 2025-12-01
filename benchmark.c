#define _DEFAULT_SOURCE
#include "cachemap.h"
#include <stdlib.h>
#include <unistd.h>

#define C 1024

static uint32_t
hash(const void *a_, uint32_t b)
{
    return *(uint32_t *)a_ % b;
}

static int
compare(const void *a_, const void *b_)
{
    uint32_t a = *(uint32_t *)a_;
    uint32_t b = *(uint32_t *)b_;
    if (a < b) return -1;
    if (a > b) return +1;
    return 0;
}

static cm_cache *
new_cache(void)
{
    cm_cache *cm;
    unsigned char *memory;
    void *hashmap;
    void *cache;

    size_t hashmap_bytes, cache_bytes;
    cm_required_bytes(sizeof(uint32_t), C, &hashmap_bytes, &cache_bytes);

    memory = malloc(sizeof(cm_cache) + hashmap_bytes + cache_bytes);

    cm = (cm_cache *)&memory[0];
    hashmap = &memory[sizeof(cm_cache)];
    cache = &memory[sizeof(cm_cache) + hashmap_bytes];

    cm_init(cm, sizeof(uint32_t), hash, compare, NULL);
    cm_set_size(cm, C, NULL, NULL);
    cm_set_data(cm, hashmap, cache);
    return cm;
}

static void
free_cache(cm_cache *cm)
{
    free(cm);
}

static uint32_t
warmup_size(uint32_t size)
{
    return (size < 1000) ? 1000 : size;
}

static double
cyclic_access(uint32_t repeat, uint32_t size)
{
    bool put;
    uint32_t hits = 0;
    cm_cache *cm = new_cache();

    for (uint32_t j = 0, el = 0; j < warmup_size(size); j++) {
        cm_get_or_put_key(cm, &el, &put);
        el = (el + 1) % size;
    }

    for (uint32_t i = 0; i < repeat; i++) {
        // measure
        for (uint32_t j = 0; j < size; j++) {
            cm_get_or_put_key(cm, &j, &put);
            hits += !put;
        }
    }

    free_cache(cm);
    return (hits * 100.0f) / (repeat * size);
}

static uint32_t
entropy(void)
{
    uint32_t r;
    getentropy(&r, sizeof(r));
    return r;
}

static double
mixed_access(uint32_t hot)
{
    bool put;
    uint32_t hits = 0;

    cm_cache *cm = new_cache();

    uint32_t hot_size = 0.5 * C;
    uint32_t cold_size = 4 * C;

    for (uint32_t j = 0; j < warmup_size(hot_size + cold_size); j++) {
        uint32_t el;
        if (entropy() % 100 < hot) {
            el = (entropy() % hot_size);
        } else {
            el = hot_size + (entropy() % cold_size);
        }

        cm_get_or_put_key(cm, &el, &put);
    }

    for (uint32_t j = 0; j < 100000; j++) {
        uint32_t el;
        if (entropy() % 100 < hot) {
            el = 0 + (entropy() % hot_size);
        } else {
            el = hot_size + (entropy() % cold_size);
        }

        cm_get_or_put_key(cm, &el, &put);
        hits += !put;
    }

    free_cache(cm);
    return (hits * 100.0f) / 100000;
}

static double
zipf_access(uint32_t alpha)
{
    (void)alpha;
    // cm_flush(cm);
    return 0.0;
}

int
main()
{
    // int a = 0;
    // int b = 0;
    // int c = 0;
    // for (int i = 0; i < 4096; i++) {
    //     int r = 
    //     if (r == 0) a++;
    //     else if (r == 1) b++;
    //     else c++;
    // }
    // printf("%g, %g, %g\n", ((double)a)/(a+b+c), ((double)b)/(a+b+c), ((double)c)/(a+b+c));

    printf("                    LRU  FIFO Adaptive-LRU\n");
    printf("Streaming 2xC     %5.1f  \n", cyclic_access(5, 2*C));
    printf("Large Loop 1.25xC %5.1f  \n", cyclic_access(10, 1.25*C));
    printf("Small Loop 0.5xC  %5.1f  \n", cyclic_access(20, 0.5*C));
    printf("Mixed 80/20       %5.1f  \n", mixed_access(80));
    printf("Zipf a=1.0        %5.1f  \n", zipf_access(1.0));
    printf("Zipf a=1.1        %5.1f  \n", zipf_access(1.1));
}
