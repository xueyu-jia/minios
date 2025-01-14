#pragma once

#include <klib/stdint.h>
#include <minios/spinlock.h>
#include <list.h>

// 缓冲块头部，记录了缓冲块的信息
typedef struct buf_head {
    u32 count;    // buffer使用计数
    u32 dev;      // 设备号 只有 busy=true 时才有效
    u32 block;    // 扇区号 只有 busy=true 时才有效
    void *buffer; // 该缓冲块的起始地址，为 cache 的地址
    struct list_node b_lru;
    struct list_node b_hash;
    int b_size; // buffer size in byte
    int b_flush;
    int b_state;
    spinlock_t lock;
    u8 used; // 该缓冲块是否被使用 0 未被使用；1 已经被使用；
} buf_head;

extern int buffer_debug;
extern struct file_operations blk_file_ops;

void init_buffer(int num_block);

//! buffer api for detailed fs impl
buf_head *bread(u32 dev, u32 block);
void mark_buff_dirty(buf_head *bh);
void brelse(buf_head *bh);
void sync_buffers(int flush_delay);
struct super_block;
void rw_buffer(int iotype, buf_head *bh, int length);
void map_bh(buf_head *bh, struct super_block *sb, u32 block);

#ifdef BUFFER_SYNC_TASK
void bsync_service();
#endif
