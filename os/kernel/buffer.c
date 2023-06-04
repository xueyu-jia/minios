#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/kmalloc.h"
#include "../include/fs_misc.h"
#include "../include/fs_const.h"
#include "../include/hd.h"

/* 缓冲块状态，
CLEAN表示缓冲块数据与磁盘数据同步，
UNUSED表示数据尚未写入，数据无效；
DIRTY表示缓冲块内为脏数据 */
/* enum buf_state
{
    UNUSED,
    CLEAN,
    DIRTY
}; */
// 缓冲块头部，记录了缓冲块的信息
typedef struct buf_head
{
    u32 count;                 // 为以后优化预留
    u32 dev, block;            // 设备号，扇区号(只有busy=true时才有效)
    void *buffer;              // 该缓冲块的起始地址, 为cache的地址
    struct buf_head *pre_lru;  // LRU链表指针，指向LRU链表的前一个元素
    struct buf_head *nxt_lru;  // 指向下一个缓冲块头部
    struct buf_head *pre_hash; // hash表链表中的前一项
    struct buf_head *nxt_hash; // hash表链表中的后一项
    u8 used;                   // 该缓冲块是否被使用 0 未被使用；1 已经被使用；
    u8 dirty;                  // 该缓冲块是否是脏的 0 clean; 1 dirty
} buf_head;
/*用于记录LRU链表的数据结构*/
struct buf_lru_list
{
    struct buf_head *lru_head; // buffer head组成的双向链表的头
    struct buf_head *lru_tail; // buffer head组成的双向链表的尾
};
#define NR_HASH 19
// 计算hash值
#define HASH_CODE(dev, block) ((block ^ dev) % NR_HASH)

/* struct buf_hash_head
{
    struct buf_head *lru_head; // buffer head组成的双向链表的头
    struct buf_head *lru_tail; // buffer head组成的双向链表的尾
}; */
static struct buf_lru_list lru_list;
static struct buf_head *buf_hash_table[NR_HASH];

/*****************************************************************************
 *                                init_buffer
 *****************************************************************************/
/**
 * 初始化缓冲区，分配好缓冲区buf head和缓冲区的内存数据块，
 * @param[in] num_block  缓冲区数据块的块数
 *
 *****************************************************************************/
void init_buffer(int num_block)
{
    // 申请buffe head和buffer block的内存，初始化lru 双向链表
    buf_head *pre = kern_kzalloc(sizeof(buf_head));
    pre->buffer = kern_kzalloc(BLOCK_SIZE);
    lru_list.lru_head = pre;
    for (int i = 0; i < num_block - 1; i++)
    {
        pre->nxt_lru = kern_kzalloc(sizeof(buf_head));
        pre->nxt_lru->buffer = kern_kzalloc(BLOCK_SIZE);
        pre->nxt_lru->pre_lru = pre;
        pre = pre->nxt_lru;
    }
    pre->nxt_lru = lru_list.lru_head;
    lru_list.lru_head->pre_lru = pre;
    lru_list.lru_tail = pre;
}
/*****************************************************************************
 *                                find_buffer
 *****************************************************************************/
/**
 * 从hash表中查找缓冲区头
 *
 * @param[in] dev  the block device id
 * @param[in] block  the block id in block device
 * @return  Ptr to buf_head if founding in hash table, NULL if not founding in hash table.
 *
 *****************************************************************************/
static struct buf_head *find_buffer(int dev, int block)
{
    int ihash = HASH_CODE(dev, block);
    buf_head *tmp = buf_hash_table[ihash];
    for (; tmp != NULL; tmp = tmp->nxt_hash)
    {
        if (tmp->dev == dev && tmp->block == block)
            return tmp;
    }
    return NULL;
}
/*****************************************************************************
 *                                rm_bh_lru
 *****************************************************************************/
/**
 * 从lru list双向链表中删除
 *
 * @param[in] bh   Ptr to buf head
 *
 *****************************************************************************/
static void rm_bh_lru(buf_head *bh)
{
    bh->pre_lru->nxt_lru = bh->nxt_lru;
    bh->nxt_lru->pre_lru = bh->pre_lru;
    if (bh == lru_list.lru_tail)
    {
        lru_list.lru_tail = bh->pre_lru;
    }
}

/*****************************************************************************
 *                                get_bh_lru
 *****************************************************************************/
/**
 * 从lru list表中获取一个最近少使用的buf_head
 *
 * @return         Ptr to buf_head.
 *
 *****************************************************************************/
static buf_head *get_bh_lru()
{
    // lru list的头部就是一个最近最少未使用的buf head
    // 需要将buf head 从lru双向链表中“摘”出来
    buf_head *bh = lru_list.lru_head;
    lru_list.lru_head = bh->nxt_lru;
    rm_bh_lru(bh);
    return bh;
}
/*****************************************************************************
 *                                put_bh_lru
 *****************************************************************************/
/**
 * 将一个buf head 放进lru的双向链表的尾部
 * @param[in] bh   Ptr to buf head
 *
 *****************************************************************************/
static void put_bh_lru(buf_head *bh)
{
    // lru list的尾部是一个最近刚刚使用的buf head，因此需要将bh添加进队尾
    bh->nxt_lru = lru_list.lru_head;
    bh->pre_lru = lru_list.lru_tail;

    lru_list.lru_head->pre_lru = bh;
    lru_list.lru_tail->nxt_lru = bh;
    lru_list.lru_tail = bh;
}
/*****************************************************************************
 *                                rm_bh_hashtbl
 *****************************************************************************/
/**
 * 将一个buf head从hash table[]中移除
 * @param[in] bh   Ptr to buf head
 *
 *****************************************************************************/
static void rm_bh_hashtbl(buf_head *bh)
{
    if (bh->pre_hash == NULL)
    { // 说明它在hash tble的散列链表中是表头
        buf_hash_table[HASH_CODE(bh->dev, bh->block)] = bh->nxt_hash;
        bh->nxt_hash->pre_hash = NULL;
    }
    else
    {
        bh->pre_hash->nxt_hash = bh->nxt_hash;
        if (bh->nxt_hash != NULL)
            bh->nxt_hash->pre_hash = bh->pre_hash;
    }
    bh->nxt_hash = NULL;
    bh->pre_hash = NULL;
}
/*****************************************************************************
 *                                put_bh_hashtbl
 *****************************************************************************/
/**
 * 将一个buf head加入到hash table[]中
 * @param[in] bh   Ptr to buf head
 *
 *****************************************************************************/
static void put_bh_hashtbl(buf_head *bh)
{
    int ihash = HASH_CODE(bh->dev, bh->block);
    if (buf_hash_table[ihash] == NULL)
    {
        buf_hash_table[ihash] = bh;
        bh->pre_hash = NULL;
        bh->nxt_hash = NULL;
        return;
    }
    buf_head *tmp = buf_hash_table[ihash];
    for (; tmp->nxt_hash != NULL; tmp = tmp->nxt_hash)
        ;
    bh->pre_hash = tmp;
    tmp->nxt_hash = bh;
    bh->nxt_hash = NULL;
}
/*****************************************************************************
 *                                getblk
 *****************************************************************************/
/**
 * 获取一个缓冲区块,如果dev block确定的缓冲区块已经在hash tbl中，则从hash tbl中返回buf head。
 * 若没在hash tbl中则从lru表中取出一个最近未使用的buf head（新分配一个buf head），此时会暂时将buf head从lru表中删除，
 * 若该buf head之前被用过则从hash tbl中删除掉，如果buf head的数据修改过并且还没同步到硬盘 则写入到硬盘
 * @param[in] dev  the block device id
 * @param[in] block  the block id in block device
 * @return         Ptr to buf_head.
 *
 *****************************************************************************/
buf_head *getblk(int dev, int block)
{
    buf_head *bh = find_buffer(dev, block);
    // 如果在hash table中能找到，则直接返回
    if (bh)
        return bh;
    // 从lru 表中获取一个最近未使用的buf head
    bh = get_bh_lru();
    // 如果它被用过则，从hash表中删掉它
    if (bh->used)
    {
        rm_bh_hashtbl(bh);
        bh->used = 0;
    }
    // 如果是脏块就立即写回硬盘
    if (bh->dirty)
    {
        WR_BLOCK_SCHED(bh->dev, bh->block, bh->buffer);
        bh->dirty = 0;
    }
    bh->dev = dev;
    bh->block = block;
}
int buf_read_block(int dev, int block, int pid, void *buf)
{
    buf_head *bh = getblk(dev, block);
    // 若used == 1，说明已经在hash tbl中了，buffer中也有数据了
    if (bh->used)
    {
        memcpy(buf, bh->buffer, BLOCK_SIZE);
        // 从lru双向链表中删除
        rm_bh_lru(bh);
    }
    else
    {
        // 该buf head是一个新分配的，此时应该从硬盘读数据进来
        RD_BLOCK_SCHED(dev, block, bh->buffer);
        memcpy(buf, bh->buffer, BLOCK_SIZE);
        // 标记为已被使用
        bh->used = 1;
        // 把它放进hash tbl中
        put_bh_hashtbl(bh);
    }
    // 把它放进lru的尾部，表示刚被用过
    put_bh_lru(bh);
    return 0;
}
int buf_write_block(int dev, int block, int pid, void *buf)
{
    buf_head *bh = getblk(dev, block);
    memcpy(bh->buffer, buf, BLOCK_SIZE);
    bh->dirty = 1;
    if (bh->used)
    {
        // 把它从lru中删除，本函数执行结束时再加到lru尾部
        rm_bh_lru(bh);
    }
    else
    {
        // 标记未被用过
        bh->used = 1;
        // 放进hash tbl中
        put_bh_hashtbl(bh);
    }
    // 把它放进lru的尾部，表示刚被用过
    put_bh_lru(bh);
    return 0;
}
