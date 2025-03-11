#pragma once

#include <klib/stdint.h>
#include <klib/sys/types.h>
#include <minios/spinlock.h>

typedef enum { EMPTY, PARTIAL, FULL } slab_type_t;

typedef struct kmem_cache kmem_cache_t;
typedef struct kmem_slab kmem_slab_t;
typedef struct kmem_list kmem_list_t;

#define MIN_BUFF_ORDER 3
#define MAX_BUFF_ORDER 11
#define MAX_EMPTY 2
#define CACHE_NAME_LEN 32
#define SLAB_BLOCK_SIZE PGSIZE
#define KMEM_CACHES_NUM (MAX_BUFF_ORDER - MIN_BUFF_ORDER + 1)

typedef uint8_t bitmap_entry_t;

#define BITMAP_ENTRY_BITS (sizeof(bitmap_entry_t) * 8)
#define BITMAP_EMPTY ((bitmap_entry_t)0)
#define BITMAP_FULL ((bitmap_entry_t)~0)

struct kmem_list {
    kmem_slab_t *head;
    spinlock_t lock;
};

struct kmem_slab {
    size_t used_obj;
    slab_type_t type;
    bitmap_entry_t *bitmap;
    uintptr_t objects;
    struct kmem_slab *next;
    struct kmem_cache *cache;
};

struct kmem_cache {
    char name[32];
    size_t obj_size;
    size_t obj_order;

    size_t nr_bitmap;
    size_t obj_per_slab;

    kmem_list_t list[3];
    size_t slab_count[3];
};

void init_cache();

phyaddr_t slab_alloc(size_t size);
void slab_free(phyaddr_t addr);
