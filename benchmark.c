#define _DEFAULT_SOURCE
#include "lru-cache.h"
#include <stdlib.h>
#include <unistd.h>

#define C 1024

uint32_t hash(const void *a_, uint32_t b)
{
    return *(uint32_t *)a_ % b;
}

int compare(const void *a_, const void *b_)
{
    uint32_t a = *(uint32_t *)a_;
    uint32_t b = *(uint32_t *)b_;
    if (a < b) return -1;
    if (a > b) return +1;
    return 0;
}

uint32_t warmup_size(uint32_t size)
{
    return (size < 1000) ? 1000 : size;
}

double cyclic_access(cm_cache *cm, uint32_t repeat, uint32_t size)
{
    bool put;
    uint32_t hits = 0;
    cm_flush(cm);

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

    return (hits * 100.0f) / (repeat * size);
}

uint32_t entropy(void)
{
    uint32_t r;
    getentropy(&r, sizeof(r));
    return r;
}

double mixed_access(cm_cache *cm, uint32_t hot)
{
    bool put;
    uint32_t hits = 0;
    cm_flush(cm);

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
            el = (entropy() % hot_size);
        } else {
            el = hot_size + (entropy() % cold_size);
        }

        cm_get_or_put_key(cm, &el, &put);
        hits += !put;
    }

    return (hits * 100.0f) / 100000;
}

double zipf_access(cm_cache *cm, uint32_t alpha)
{
    (void)alpha;
    cm_flush(cm);
    return 0.0;
}

int main()
{
    size_t hashmap_bytes, cache_bytes;
    cm_cache cm;
    cm_init(&cm, sizeof(uint32_t), hash, compare, NULL);

    cm_set_size(&cm, C, &hashmap_bytes, &cache_bytes);
    void *hashmap = malloc(hashmap_bytes);
    void *cache = malloc(cache_bytes);
    cm_set_data(&cm, hashmap, cache);

    printf("                    LRU  FIFO Adaptive-LRU\n");
    printf("Streaming 2xC     %5.1f  \n", cyclic_access(&cm, 5, 2*C));
    printf("Large Loop 1.25xC %5.1f  \n", cyclic_access(&cm, 10, 1.25*C));
    printf("Small Loop 0.5xC  %5.1f  \n", cyclic_access(&cm, 20, 0.5*C));
    printf("Mixed 80/20       %5.1f  \n", mixed_access(&cm, 80));
    printf("Zipf a=1.0        %5.1f  \n", zipf_access(&cm, 1.0));
    printf("Zipf a=1.1        %5.1f  \n", zipf_access(&cm, 1.1));
}
