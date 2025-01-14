#pragma once

#include <klib/stdint.h>
#include <klib/stddef.h>

#define MAX_MBLOCK 1024 // 内核态下已分配记录表记录数量上限
#define MAX_KFREE 1024  // 内核态下空闲分区表分区数量上限

typedef struct {
    uintptr_t addr;
    size_t size;
} mblkent_t;

// 核心态下内核地址空间物理内存空闲分区表，用于小内存分配
typedef struct free_mem_info {
    size_t count;                   // 分区数目
    mblkent_t mem_table[MAX_KFREE]; // 结构体数组每一项记录一个分区信息
    size_t total_mem_size;
    size_t current_mem_size;
} free_mem_info_t;

// 已经kmalloc的内存块的记录表，供kfree时查找
typedef struct used_mem_info {
    size_t count;
    mblkent_t mtable[MAX_MBLOCK];
} used_mem_info_t;

void kmem_init();

void insert_used_memblk(u32 addr, u32 size);
size_t get_kmalloc_size(u32 addr);
u32 kmalloc_over4k(u32 size);
u32 kfree_over4k(u32 addr);

extern free_mem_info_t kmem;
