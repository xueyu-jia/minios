#pragma once

#include <stdint.h>

#define MAX_FREE 1024   // 空闲分区表分区数量上限
#define MAX_MBLOCK 1024 // 已分配记录表记录数量上限
#define MAX_VPAGE 128   // 空闲页面表页面数量上限
// #define page_size 4096
// #define Heap_Base  0x48000000
// #define Heap_Limit 0x88000000

typedef struct heap_ptr // 堆指针，base表示堆底部，limit表示堆顶
{
    uint32_t heap_lin_base;
    uint32_t heap_lin_limit;
} heap_ptr;

/*
typedef struct vpage //4k的逻辑页
{
    uint32_t vaddr;
    struct vpage_t *next;
}vpage;
*/

typedef struct vpage_list // 4k逻辑页线性表
{
    uint32_t vpage[MAX_VPAGE]; // 数组每一个元素记录空闲4k逻辑页面的起始地址
    uint32_t vpage_count;      // 线性表上页面个数
} vpage_list;

// 虚拟内存空闲分区表表项元素
typedef struct free_vblock {
    uint32_t vaddr; // 每个分区起始地址
    uint32_t size;  // 分区大小
} free_vblock;

// 虚拟内存空闲分区表（每一个分区都小于4K）
typedef struct free_vmem {
    uint32_t count;                   // 分区数目
    free_vblock vmem_table[MAX_FREE]; // 记录空闲分区的结构体数组
} free_vmem;

// 已经malloc的虚拟内存块记录表表项，供free时使用
typedef struct malloc_block {
    uint32_t vaddr;
    uint32_t size;
} malloc_block;

// 已经malloc的虚拟内存块记录表，供free时使用
typedef struct malloced {
    uint32_t count;                  // 已经malloc的内存块的数目
    malloc_block mtable[MAX_MBLOCK]; // 数组每一项记录已经malloc的内存块的起始地址
} malloced;

// 供用户malloc/free的函数
uint32_t malloc(uint32_t size); // 用户程序执行的malloc
uint32_t free(uint32_t vaddr);  // 用户程序执行的free

void scan_vtable();
void scan_malloc_table();
