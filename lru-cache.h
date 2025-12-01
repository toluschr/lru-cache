#ifndef CACHE_MAP_H_
#define CACHE_MAP_H_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define CM_FNV1A64_IV 0xcbf29ce484222325ull
#define CM_DJB2_IV 5381ull

#define CM_NIL UINT32_MAX

#define CM_IS_ENTRY_EMPTY

typedef struct {
    uint32_t lru;
    uint32_t mru;
    uint32_t clru;
    uint32_t cmru;
    char key[];
} cm_entry;

typedef void cm_destroy(void *a, uint32_t index);
typedef int cm_compare(const void *a, const void *b);
typedef uint32_t cm_hash(const void *a, uint32_t nmemb);

typedef struct {
    uint32_t *hashmap;
    void *cache;

    cm_hash *hash;
    cm_compare *compare;
    cm_destroy *destroy;

    uint16_t psel_ctr;
    uint8_t psel_bit;
    uint8_t leader_set_size;

    uint32_t size;
    uint32_t nmemb;
    uint32_t try_nmemb;

    uint32_t lru;
    uint32_t mru;
} cm_cache;

uint64_t cm_fnv1a64_step(uint64_t state, const void *data, size_t size);
uint64_t cm_djb2_step(uint64_t state, const void *data, size_t size);

int cm_align(uint32_t size, uint32_t align, uint32_t *size_a_);
int cm_required_bytes(uint32_t size_a, uint32_t nmemb, size_t *hashmap_bytes, size_t *cache_bytes);

cm_entry *cm_entry_ptr(cm_cache *cm, uint32_t slot);

void cm_move_chain(cm_cache *cm, uint32_t slot, uint32_t old_hash, uint32_t new_hash);

void cm_make_lru(cm_cache *cm, uint32_t slot);
void cm_make_mru(cm_cache *cm, uint32_t slot);

int cm_init(cm_cache *cm, uint32_t size_a, cm_hash *hash, cm_compare *compare, cm_destroy *destroy);
int cm_set_size(cm_cache *cm, uint32_t nmemb, size_t *hashmap_bytes, size_t *cache_bytes);
int cm_set_data(cm_cache *cm, void *hashmap, void *cache);

void cm_flush(cm_cache *cm);
bool cm_is_full(cm_cache *cm);

uint32_t cm_put_key(cm_cache *cm, const void *key);
uint32_t cm_get_or_put_key(cm_cache *cm, const void *key, bool *put);

#endif // CACHE_MAP_H_
