#pragma once

#include <minios/mempage.h>
#include <minios/spinlock.h>
#include <klib/stdint.h>
#include <klib/stddef.h>

#define BUDDY_ORDER_LIMIT 11

struct free_area {
    memory_page_t *free_list;
    u32 free_count;
};

typedef struct buddy {
    spinlock_t lock;
    struct free_area free_area[BUDDY_ORDER_LIMIT];
    size_t total_mem;
    size_t used_mem;
} buddy_t;

extern buddy_t *bud;
extern size_t buddy_order_size[BUDDY_ORDER_LIMIT];

/*!
 * \brief init buddy system with available physical memory in [pa_lo, pa_hi)
 *
 * \param bud buddy object to initialize
 * \param pa_lo lower bound of physical memory
 * \param pa_hi upper bound of physical memory
 *
 * \note pa_lo and pa_hi simply restrict the bounds and our impl will select the eligible ones from
 * multiboot info mmap
 *
 * \attention memory block assigned to a buddy object is not guaranteed to be exclusive, ensures it
 * in the caller side
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
 * \brief get best-fit order of the given size
 *
 * \return minimum order that fits the requested size
 * \retval -1 failed to found the order
 *
 * \note actually we will not return but abort if failed to match
 */
int buddy_fit_order(size_t size);

/*!
 * \brief allocate 2^order sized memory block from buddy system
 *
 * \param bud buddy object
 * \param order order of the requested memory block
 *
 * \return page object indicates the first page of the memory block with PAGESTATE_ALLOCATED state
 * \retval NULL out of memory
 */
memory_page_t *buddy_alloc(buddy_t *bud, int order);

/*!
 * \brief allocated 2^order sized memory block in the restricted range from buddy system
 *
 * \param bud buddy object
 * \param order order of the requested memory block
 * \param addr_min lower bound for the allocated memory block
 * \param addr_max upper bound for the allocated memory block, inclusive
 *
 * \return page object indicates the first page of the memory block with PAGESTATE_ALLOCATED state
 * \retval NULL out of memory
 */
memory_page_t *buddy_alloc_restricted(buddy_t *bud, int order, uintptr_t addr_min,
                                      uintptr_t addr_max);

/*!
 * \brief free memory block to buddy system
 *
 * \param bud buddy object
 * \param blk_head object of the first page of the allocated memory block
 */
void buddy_free(buddy_t *bud, memory_page_t *blk_head);
