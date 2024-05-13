// 缓冲块头部，记录了缓冲块的信息
#ifndef BUFFER_H
#define BUFFER_H
#include "type.h"
#include "spinlock.h"
#include "list.h"
typedef struct buf_head
{
    u32 count;                 // buffer使用计数
    u32 dev, block;            // 设备号，扇区号(只有busy=true时才有效)
    void *buffer;              // 该缓冲块的起始地址, 为cache的地址
    // struct buf_head *pre_lru;  // LRU链表指针，指向LRU链表的前一个元素
    // struct buf_head *nxt_lru;  // 指向下一个缓冲块头部
    // struct buf_head *pre_hash; // hash表链表中的前一项
    // struct buf_head *nxt_hash; // hash表链表中的后一项
	struct list_node b_lru;
	struct list_node b_hash;
	// struct list_node b_dirty;
    int b_size; // buffer size in byte
	int b_flush;
    int b_state;
    SPIN_LOCK lock;
    u8 used;                   // 该缓冲块是否被使用 0 未被使用；1 已经被使用；
    // u8 dirty;                  // 该缓冲块是否是脏的 0 clean; 1 dirty 合并到state中
} buf_head;
/* int buf_write_block(int dev, int block, int pid, void *buf);
int buf_read_block(int dev, int block, int pid, void *buf); */
/**********初始化buffer函数********************/
void init_buffer(int num_block);
/*#define BUF_RD_BLOCK(dev,block_nr,fsbuf) buf_read_block(dev,block_nr,proc2pid(p_proc_current),fsbuf);
#define BUF_WR_BLOCK(dev,block_nr,fsbuf) buf_write_block(dev,block_nr,proc2pid(p_proc_current),fsbuf); */
extern int buffer_debug; 
/********buffer向文件系统提供的API********/
PUBLIC buf_head *bread(int dev, int block);
PUBLIC void mark_buff_dirty(buf_head *bh);
PUBLIC void brelse(buf_head *bh);
PUBLIC void sync_buffers(int flush_delay);
struct super_block;
PUBLIC void rw_buffer(int iotype, buf_head* bh);
PUBLIC void map_bh(buf_head *bh, struct super_block *sb, u32 block);

// #define BUFFER_SYNC_TASK
#ifdef BUFFER_SYNC_TASK
    void bsync_service();
#endif
extern struct file_operations blk_file_ops;
#endif