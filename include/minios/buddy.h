#pragma once

#include <minios/mempage.h>
#include <minios/spinlock.h>
#include <klib/stdint.h>
#include <klib/stddef.h>

#define BUDDY_ORDER_LIMIT 11

struct free_area {
    memory_page_t *free_list; // 空闲页块线性表
    u32 free_count;           // 空闲页块数目
};

typedef struct buddy {
    spinlock_t lock;
    struct free_area free_area[BUDDY_ORDER_LIMIT];
    size_t total_mem;
    size_t used_mem;
} buddy_t;

extern buddy_t *bud;
extern size_t block_size[BUDDY_ORDER_LIMIT];

/*!
 * \brief init buddy system with available physical memory in [pa_lo, pa_hi)
 */
void buddy_init(buddy_t *bud, phyaddr_t pa_lo, phyaddr_t pa_hi);

/*!
 * \brief join pages in [pa_lo, pa_hi) to buddy free list
 *
 * \param bud buddy object
 * \param pa_lo lower bound of physical address
 * \param pa_hi upper bound of physical address
 * \param strict if true, only phantom pages will be joined
 *
 * \return total memory size of the newly joined pages
 * \retval -1 unknown error
 */
ssize_t buddy_join_blk(buddy_t *bud, phyaddr_t pa_lo, phyaddr_t pa_hi, bool strict);

/*!
 * \brief allocate 2^order sized memory block from buddy system
 */
memory_page_t *buddy_alloc(buddy_t *bud, int order);

/*!
 * \brief free memory block to buddy system
 */
void buddy_free(buddy_t *bud, memory_page_t *blk_head);
