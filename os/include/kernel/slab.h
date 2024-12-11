#ifndef KERNEL_SLAB_H_
#define KERNEL_SLAB_H_

#include <kernel/type.h>
#include <klib/spinlock.h>

typedef enum slab_type { EMPTY, PARTIAL, FULL } slab_type_t; // slab的类型
typedef struct kmem_cache kmem_cache_t;
typedef struct kmem_slab kmem_slab_t;
typedef struct kmem_list kmem_list_t;

typedef unsigned char bitmap_entry_t; // 位图每块的类型

#define BITMAP_EMPTY 0x00 // 位图的一块全未被标记。初始化位图使用
#define BITMAP_FULL \
    0xff // 位图的一块全被标记。分配对象时使用，跳过全部被标记的块
#define BITMAP_ENTRY_BITS 8 // 位图的每一块能够表示的对象数目

#define pow(order) (1 << order) // 计算2的order次幂
#define MIN_BUFF_ORDER 3        // 最小对象的order，8B
#define MAX_BUFF_ORDER 11       // 最大对象的order，2048B
#define MAX_EMPTY 2             // metpy链中最大slab数目
#define CACHE_NAME_LEN 32       // Cache 名字长度
#define BLOCK_SIZE 4096         // page大小
#define KMEM_CACHES_NUM 9       // Cache的数目

#define LEFTSHIFT(n) (1 << (n))
// 将address向上(高地址)对齐,对齐标准为(2^n)
#define ROUNDUP(address, n) \
    (((address) + LEFTSHIFT(n) - 1) & ~(LEFTSHIFT(n) - 1))

struct kmem_list {
    kmem_slab_t *head;
    SPIN_LOCK lock;
};

/*
    slab的空间视图
        start                   (4K对齐的)
            kmem_slab           (slab的记录数据结构)
            bitmap              (位图，kmem_slab.bitmap
   指向该地址，长度为kmem_cache.bitmap_length) padding
   (填充区，不使用,用以保证objects1是对齐的) objects  1
   (用于分配，开头地址按照kmem_cache.objsize对齐,kmem_slab.objects指向该地址)
            ...
            objects  n
        end                     (start + 4K - 1)
*/
struct kmem_slab {
    u32 used_obj;             // slab中已经使用的对象个数
    slab_type_t type;         // slab在cache中的类型
    bitmap_entry_t *bitmap;   // 记录位图的首地址
    u32 objects;              // 记录object的首地址
    struct kmem_slab *next;   // 指向下一个slab，初始化为NULL
    struct kmem_cache *cache; // 指向管理slab的cache
};

// Cache structure
struct kmem_cache {
    char name[32]; // cache名称
    u32 objsize;   // cache管理的对象的大小
    u32 objorder;  // cache管理的对象order. objorder=log2(objsize)

    u32 bitmap_length; // 每个slab的位图块个数
    u32 obj_per_slab;  // 每个slab管理的对象个数

    kmem_list_t list[3]; // 三个链表
    u32 slab_count[3];   // 三种链表的个数
};

/*
    功能：计算指针偏移
    传入参数：
        ptr：指针起始地址
        offset：偏移量
*/
#define ptr_offset(ptr, offset) (void *)((char *)ptr + offset)

// 位图操作
/*
    分配对象后，位图对应位置置为 1
    传入参数：
        bitmap：位图起始地址
        index： 分配的对象的序号，从0开始计数
*/
#define bitmap_set_used(bitmap, index) \
    bitmap[index / BITMAP_ENTRY_BITS] |= (1 << (index % BITMAP_ENTRY_BITS))

/*
    释放对象后，位图对应位置置为 0
    传入参数：
        bitmap：位图起始地址
        index： 分配的对象的序号，从0开始计数
*/
#define bitmap_set_free(bitmap, index) \
    bitmap[index / BITMAP_ENTRY_BITS] &= ~(1 << (index % BITMAP_ENTRY_BITS))

/*
    判断要释放的地址是否已经是空闲的，避免重复释放
    传入参数：
        bitmap：位图起始地址
        index： 分配的对象的序号，从0开始计数
    返回值：
        0  要释放的地址不空闲
        1  要释放的地址已经空闲
*/
#define bitmap_is_free(bitmap, index)     \
    ((bitmap[index / BITMAP_ENTRY_BITS] & \
      (1 << (index % BITMAP_ENTRY_BITS))) == 0)

/*
    由对象个数得到位图块的个数
    传入参数：
        obj_count：对象个数
    返回值：
        位图的字节数目
*/
#define calc_bitmap_size(obj_count)                                           \
    ((obj_count / BITMAP_ENTRY_BITS + (obj_count % BITMAP_ENTRY_BITS != 0)) * \
     sizeof(bitmap_entry_t))

void init_cache();
void *kmalloc(u32 size);
int kfree(u32 object);

void printInfo(kmem_cache_t *cache);
void printInfoAll();
#endif
