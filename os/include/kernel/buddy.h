/*************************************************************
*     buddy.h     add by wang   2021.3.3
**************************************************************/
#ifndef KERNEL_BUDDY_H
#define KERNEL_BUDDY_H

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/memman.h>

#define MAX_ORDER 11        //页块链表数目


struct free_area
{
    page *free_list;  //空闲页块线性表          edit by wang 2021.3.9
    u32  free_count;     //空闲页块数目
};

typedef struct buddy
{
    struct free_area free_area[MAX_ORDER]; //伙伴系统数组，每一个数组元素代表一种内存页块
    u32 total_mem_size;                    //added by wang 2021.8.26
    u32 current_mem_size;

}buddy;

page* alloc_pages(buddy *bud, u32 order);   //分配2^order页的页块
int free_pages(buddy *bud, page *page, u32 order);

extern buddy *kbud;
extern buddy *ubud;
extern u32 block_size[11];

void buddy_init(buddy *bud, u32 bgn_addr, u32 end_addr);
int block_init(u32 bgn_addr, u32 end_addr, buddy *bud);
#endif
