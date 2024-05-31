#include "const.h"
#include "fs_const.h"
#include "proc.h"
#include "clock.h"
#include "vfs.h"
#include "hd.h"
#include "string.h"
#include "memman.h"
#include "semaphore.h"
#include "buffer.h"

int buffer_debug = 0;
#define buff_log(bh, info_type) if(buffer_debug == 1){disp_int(info_type);disp_str(":");disp_int(bh->block);disp_str(" ");}


/*用于记录LRU链表的数据结构*/
// struct buf_lru_list
// {
//     struct buf_head *lru_head; // buffer head组成的双向链表的头
//     struct buf_head *lru_tail; // buffer head组成的双向链表的尾
// };
#define NR_HASH 19
// 计算hash值
#define HASH_CODE(dev, block) ((block ^ dev) % NR_HASH)
#define BUFFER_FREE     -1
#define BUFFER_CLEAN    0
#define BUFFER_LOCKED   1
#define BUFFER_DIRTY    2
#define NR_BUFFER_STATE 3

#define BUF_SYNC_DELAY_TICK	5
#define BUF_SYNC_RATELIMIT  20
#define BUF_SYNC_CYCLE_TICK 20
/* struct buf_hash_head
{
    struct buf_head *lru_head; // buffer head组成的双向链表的头
    struct buf_head *lru_tail; // buffer head组成的双向链表的尾
}; */
// static struct buf_lru_list buf_lru_list;
// static struct buf_head *buf_hash_table[NR_HASH];
static list_head buf_lru_list[NR_BUFFER_STATE];
static int buf_lru_cnt[NR_BUFFER_STATE] = {0};
static SPIN_LOCK buf_lru_lock;
static SPIN_LOCK buf_lock;

static list_head buf_free_list;
// static list_head dirty_list;
static list_head buf_hash_table[NR_HASH];
static int buf_dirty_nfrac = 20;
static int buf_block_cnt;
// static SPIN_LOCK buf_dirty_lock;
PRIVATE struct Semaphore buf_dirty_sem;

/*****************************************************************************
 *                                rm_bh_lru
 *****************************************************************************/
/**
 * 从lru list双向链表中删除
 *
 * @param[in] bh   Ptr to buf head
 *
 *****************************************************************************/
static inline void rm_bh_lru(buf_head *bh)
{
	list_remove(&bh->b_lru);
    if(bh->b_state != BUFFER_FREE){
        buf_lru_cnt[bh->b_state]--;
    }
}

void sync_buffers(int flush_delay);
PRIVATE void update_bh_lru(buf_head *bh, int state);
/*****************************************************************************
 *                                get_bh_lru
 *****************************************************************************/
/**
 * 从lru list表中获取一个最近少使用的buf_head   require: buf_lru_lock
 *
 * @return         Ptr to buf_head.
 *
 *****************************************************************************/
static inline buf_head *get_bh_lru()
{
    // lru list的头部就是一个最近最少未使用的buf head
    // 需要将buf head 从lru双向链表中“摘”出来
    // buf_head *bh = lru_list.lru_head;
    // lru_list.lru_head = bh->nxt_lru;
// retry:
    // acquire(&buf_lru_lock);
	buf_head *bh = list_front(&buf_free_list, struct buf_head, b_lru);
    if(bh && bh->count){
        bh = NULL;
    }
    if(bh == NULL) {
        bh = list_front(&buf_lru_list[BUFFER_CLEAN], struct buf_head, b_lru);
    }
    if(bh && bh->count){
        bh = NULL;
    }
    // release(&buf_lru_lock);
    if(bh == NULL) {
        // sync_buffers(0);
        // goto retry;
        bh = list_front(&buf_lru_list[BUFFER_DIRTY], struct buf_head, b_lru);
    }
    if(bh == NULL) {
        disp_str("error: cant find buffer\n");
    }
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
static void put_bh_lru(buf_head *bh, int state)
{
    // lru list的尾部是一个最近刚刚使用的buf head，因此需要将bh添加进队尾
	list_add_last(&bh->b_lru, &buf_lru_list[state]);
    buf_lru_cnt[state]++;
}

static void update_bh_lru(buf_head *bh, int state) {
    rm_bh_lru(bh);
    if(bh->b_state != state)
        bh->b_state = state;
    put_bh_lru(bh, state);
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
	list_remove(&bh->b_hash);
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
	list_add_first(&bh->b_hash, &buf_hash_table[ihash]);

}

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
	buf_head *bh = NULL;
	initlock(&buf_lru_lock,"buf_lru_lock");
    // initlock(&buf_dirty_lock,"buf_dirty_lock");
	ksem_init(&buf_dirty_sem, 0);
    buf_block_cnt = num_block;
    list_init(&buf_free_list);
    for(int i=0; i < NR_BUFFER_STATE; i++){
	    list_init(&buf_lru_list[i]);
    }
	// list_init(&dirty_list);
	for(int i = 0; i < NR_HASH; i++) {
		list_init(&buf_hash_table[i]);
	}
	for(int i = 0; i < num_block; i++) {
		bh = (buf_head*)kern_kzalloc(sizeof(buf_head));
		bh->buffer = (void*)kern_kzalloc(BLOCK_SIZE);
        bh->b_state = BUFFER_FREE;
        bh->b_size = BLOCK_SIZE;
		initlock(&bh->lock, NULL);
		list_init(&bh->b_lru);
		list_init(&bh->b_hash);
		list_add_last(&bh->b_lru, &buf_free_list);
	}
}

PUBLIC void rw_buffer(int iotype, buf_head* bh, int length) {
    // disp_str("\nbuffer:");
    // disp_int((iotype == DEV_WRITE));
    // disp_str(" blk:");
    // disp_int(bh->block);
    // disp_str(" sz:");
    // disp_int(bh->b_size);
    u64 pos = bh->block * bh->b_size;
    if(kernel_initial == 1) { // init stage no sched
        rw_blocks(iotype, bh->dev, pos, length, 
            proc2pid(p_proc_current), bh->buffer);
    }else {
        rw_blocks_sched(iotype, bh->dev, pos, length, 
            proc2pid(p_proc_current), bh->buffer);
    }
    // disp_str("\nbuffer:");
    // disp_str(" blk:");
    // disp_int(bh->block);
    // disp_str(" fin");
}

PUBLIC void map_bh(buf_head *bh, struct super_block *sb, u32 block)
{
	bh->dev = sb->sb_dev;
	bh->block = block;
	bh->b_size = sb->sb_blocksize;
}

/*****************************************************************************
 *                                find_buffer_hash
 *****************************************************************************/
/**
 * 从hash表中查找缓冲区头
 *
 * @param[in] dev  the block device id
 * @param[in] block  the block id in block device
 * @return  Ptr to buf_head if founding in hash table, NULL if not founding in hash table.
 *
 *****************************************************************************/
static struct buf_head *find_buffer_hash(int dev, int block)
{
    int ihash = HASH_CODE(dev, block);
    // buf_head *tmp = buf_hash_table[ihash];
    // for (; tmp != NULL; tmp = tmp->nxt_hash)
    // {
    //     if (tmp->dev == dev && tmp->block == block)
    //         return tmp;
    // }
	buf_head *tmp = NULL;
	list_for_each(&buf_hash_table[ihash], tmp, b_hash) {
		if (tmp->dev == dev && tmp->block == block){
            return tmp;
        }
	}
    return NULL;
}

static inline void sync_buff(buf_head *bh){
	// disp_int(bh->block);
	// disp_str(".");
    update_bh_lru(bh, BUFFER_LOCKED);
	int tick = ticks;
	// if(kernel_initial){
	// 	WR_BLOCK(bh->dev, bh->block, bh->buffer);
	// }else{
	// 	WR_BLOCK_SCHED(bh->dev, bh->block, bh->buffer);
	// }
    rw_buffer(DEV_WRITE, bh, bh->b_size);
	// disp_int(ticks-tick);
	// disp_str(" ");
    bh->b_flush = 0;
    update_bh_lru(bh, BUFFER_CLEAN);
}

PRIVATE int flush_tick;
PUBLIC void sync_buffers(int flush_delay) {
    int nr_dirty = 0;
    buf_head *bh = NULL;
    // disp_str("\nbuf sync:");
    if(!(list_empty(&buf_lru_list[BUFFER_DIRTY]))){
        while(nr_dirty <= BUF_SYNC_RATELIMIT){
            bh = list_front(&buf_lru_list[BUFFER_DIRTY], buf_head, b_lru);
            if(bh == NULL)
                break;
            if(flush_delay && ticks < bh->b_flush) {
                update_bh_lru(bh, bh->b_state);
                continue;
            }
            lock_or_yield(&bh->lock);
            sync_buff(bh);
            release(&bh->lock);
            nr_dirty++;
        }
    }
    flush_tick = ticks;
}

PRIVATE void try_sync_buffers(int background) {
    lock_or_yield(&buf_lock);
    // buffer写回条件: dirty buffer数超过buffer总量的buf_dirty_nfrac% (20%)
    // 或者存在dirty buffer且距离上一次将dirty buffer同步操作的时间超过20ticks
    if((buf_lru_cnt[BUFFER_DIRTY] > buf_dirty_nfrac*buf_block_cnt/100)
        ||((ticks > flush_tick + BUF_SYNC_CYCLE_TICK) && buf_lru_cnt[BUFFER_DIRTY])){
        #ifdef BUFFER_SYNC_TASK
        if(background) {
            release(&buf_lock);
            ksem_post(&buf_dirty_sem, 1); // wake up sync task
            return;
        }
        #endif
        sync_buffers(0);
    }
    release(&buf_lock);
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
    lock_or_yield(&buf_lock);
    buf_head *bh = find_buffer_hash(dev, block);
    // 如果在hash table中能找到，则直接返回
    if (!bh)
    {
        // 从lru 表中获取一个最近未使用的buf head
        bh = get_bh_lru();
		if(bh->count) {
			disp_str("warning: still used blk\n");
		}
        // 如果是脏块就立即写回硬盘
		lock_or_yield(&bh->lock); // race with bsync
        if (bh->b_state == BUFFER_DIRTY)
        {
            sync_buff(bh);
        }
        // 如果它被用过则，从hash表中删掉它
        if (bh->used)
        {
            rm_bh_hashtbl(bh);
            bh->used = 0;
            //缓冲区重新初始化为0
            memset(bh->buffer,0,num_4K);
        }
        bh->dev = dev;
        bh->block = block;
        //放到hash 表中
		release(&bh->lock);
        put_bh_hashtbl(bh);
        update_bh_lru(bh, BUFFER_CLEAN);
    }
    //更新 lru
    update_bh_lru(bh, bh->b_state);
    release(&buf_lock);
    return bh;
}

/* int buf_read_block(int dev, int block, int pid, void *buf)
{
    buf_head *bh = getblk(dev, block);
    // 若used == 1，说明已经在hash tbl中了，buffer中也有数据了
    if (bh->used)
    {
        memcpy(buf, bh->buffer, BLOCK_SIZE);
        // 从lru双向链表中删除
        //rm_bh_lru(bh);
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
} */
/* int buf_write_block(int dev, int block, int pid, void *buf)
{
    buf_head *bh = getblk(dev, block);
    memcpy(bh->buffer, buf, BLOCK_SIZE);
    bh->dirty = 1;
    if (bh->used)
    {
        // 把它从lru中删除，本函数执行结束时再加到lru尾部
        //rm_bh_lru(bh);
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
} */

buf_head *bread(int dev, int block)
{
    buf_head *bh = getblk(dev, block);
    // 若used == 1，说明已经在hash tbl中了，buffer中也有数据了
    lock_or_yield(&bh->lock);
	// buff_log(bh, 0);
    if (!bh->used)
    {
        // 该buf head是一个新分配的，此时应该从硬盘读数据进来
		// if(kernel_initial){
		// 	RD_BLOCK(dev, block, bh->buffer);
		// }else{
		// 	RD_BLOCK_SCHED(dev, block, bh->buffer);
		// }
        rw_buffer(DEV_READ, bh, bh->b_size);
        // 标记为已被使用
        bh->used = 1;
        bh->count = 1;
    }else{
        bh->count++;
    }
    release(&bh->lock);
    return bh;
}

void mark_buff_dirty(buf_head *bh)
{
	// buff_log(bh, 1);
    lock_or_yield(&buf_lock);
    update_bh_lru(bh, BUFFER_DIRTY);
    release(&buf_lock);
}

void brelse(buf_head *bh)
{
    lock_or_yield(&buf_lock);
    update_bh_lru(bh, bh->b_state);
    release(&buf_lock);
    lock_or_yield(&bh->lock);
    if(bh->b_state == BUFFER_DIRTY) {
        int new_flush = ticks + BUF_SYNC_DELAY_TICK;
        if(new_flush > bh->b_flush) {
            bh->b_flush = new_flush;
        }
    }else {
        bh->b_flush = 0;
    }
	// buff_log(bh, 2);
    if(bh->count > 0){
        bh->count--;
    }

    if(bh->b_state == BUFFER_DIRTY && bh->count == 0){
        release(&bh->lock);
        #ifdef BUFFER_SYNC_TASK
            try_sync_buffers(1);
        #else
            try_sync_buffers(0);
        #endif
        return;
    }
    release(&bh->lock);
}


PUBLIC void bsync_service() {
	while(1) {
        ksem_wait(&buf_dirty_sem, 1);
        lock_or_yield(&buf_lock);
        sync_buffers(1);
        release(&buf_lock);
	}
}

PUBLIC int blk_file_write(struct file_desc* file, unsigned int count, const char* buf){
	u64 pos = file->fd_pos;
	struct inode *inode = file->fd_dentry->d_inode;
	u64 pos_end = min(pos + count, inode->i_size);
	int off = pos % BLOCK_SIZE;
	int rw_blk_min = (pos >> BLOCK_SIZE_SHIFT);
	int rw_blk_max = (pos_end >> BLOCK_SIZE_SHIFT);
	int chunk = min(rw_blk_max - rw_blk_min + 1,BLOCK_SIZE >> BLOCK_SIZE_SHIFT);
	int bytes_rw = 0;
	int bytes_left = count;
	int i;
	char *fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = rw_blk_min; i <= rw_blk_max; i += chunk){
		/* read/write this amount of bytes every time */
		int bytes = min(bytes_left, chunk * BLOCK_SIZE - off);
		bh = bread(inode->i_b_cdev,i);
		fsbuf = bh->buffer;
		if (pos + bytes > pos_end){
			bytes = pos_end - pos;
		}
		memcpy((void *)(fsbuf + off), (void*)(buf + bytes_rw), bytes);
		mark_buff_dirty(bh);
		off = 0;
		bytes_rw += bytes;
		pos += bytes;
		bytes_left -= bytes;
		brelse(bh);
	}
	file->fd_pos = pos;
	// if (file->fd_pos > inode->i_size) block dev size cant change
	// {
	// 	/* update inode::size */
	// 	inode->i_size = file->fd_pos;
	// }
	return bytes_rw;
}

PUBLIC int blk_file_read(struct file_desc* file, unsigned int count, char* buf){
	u64 pos = file->fd_pos;
	struct inode *inode = file->fd_dentry->d_inode;
	u64 pos_end = min(pos + count, inode->i_size);
	int off = pos % BLOCK_SIZE;
	int rw_blk_min = (pos >> BLOCK_SIZE_SHIFT);
	int rw_blk_max = (pos_end >> BLOCK_SIZE_SHIFT);
	int chunk = min(rw_blk_max - rw_blk_min + 1,BLOCK_SIZE >> BLOCK_SIZE_SHIFT);
	int bytes_rw = 0;
	int bytes_left = count;
	int i;
	char *fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = rw_blk_min; i <= rw_blk_max; i += chunk){
		/* read/write this amount of bytes every time */
		int bytes = min(bytes_left, chunk * BLOCK_SIZE - off);
		bh = bread(inode->i_b_cdev,i);
		fsbuf = bh->buffer;
		if (pos + bytes > pos_end){
			bytes = pos_end - pos;
		}
		memcpy((void*)(buf + bytes_rw), (void *)(fsbuf + off), bytes);
		off = 0;
		bytes_rw += bytes;
		pos += bytes;
		bytes_left -= bytes;
		brelse(bh);
	}
	file->fd_pos = pos;
	return bytes_rw;
}

struct file_operations blk_file_ops = {
.read = blk_file_read,
.write = blk_file_write,
.fsync = NULL
};