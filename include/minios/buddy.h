#pragma once

#include <minios/mempage.h>
#include <klib/stdint.h>

#define BUDDY_ORDER_LIMIT 11 // 页块链表数目

struct free_area {
    memory_page_t *free_list; // 空闲页块线性表
    u32 free_count;           // 空闲页块数目
};

typedef struct buddy {
    struct free_area free_area[BUDDY_ORDER_LIMIT]; // 伙伴系统数组，每一个数组元素代表一种内存页块
    u32 total_mem_size;
    u32 current_mem_size;
} buddy_t;

extern buddy_t *kbud;
extern buddy_t *ubud;
extern u32 block_size[BUDDY_ORDER_LIMIT];

memory_page_t *alloc_pages(buddy_t *bud, u32 order); // 分配2^order页的页块
int free_pages(buddy_t *bud, memory_page_t *page, u32 order);

void buddy_init(buddy_t *bud, u32 bgn_addr, u32 end_addr);
int block_init(u32 bgn_addr, u32 end_addr, buddy_t *bud);
