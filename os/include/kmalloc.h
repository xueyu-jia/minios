/*************************************************************
*     kmalloc.h     add by wang   2021.3.3
**************************************************************/



#include "type.h"

#define MAX_MBLOCK 1024         //内核态下已分配记录表记录数量上限
#define MAX_KFREE 1024          //内核态下空闲分区表分区数量上限

//核心态下内核地址空间物理内存空闲分区表表项
typedef struct free_kblock
{
    u32 addr;  //空闲分区起始地址
    u32 size;  //空闲分区大小
}free_kblock;

typedef struct free_kmem  //核心态下内核地址空间物理内存空闲分区表，用于小内存分配
{
    u32 count;                  //分区数目
    free_kblock kmem_table[MAX_KFREE];  //结构体数组每一项记录一个分区信息
    u32 total_mem_size;                //added by wang 2021.8.26
    u32 current_mem_size;
}free_kmem;


typedef struct malloc_block  //已经kmalloc的内存块的记录表表项
{
    u32 addr;
    u32 size;
} malloc_block;

typedef struct malloced    //已经kmalloc的内存块的记录表，供kfree时查找
{
    u32 count;
    malloc_block mtable[MAX_MBLOCK];
} malloced;

void kmem_init();
void malloced_insert(u32 addr, u32 size);

extern free_kmem kmem;