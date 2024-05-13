#include "hd.h"
#include "string.h"
#include "vfs.h"
#include "buffer.h"
#include "tty.h"
#include "hd.h"
#include "memman.h"
#include "mmap.h"

PUBLIC struct super_block super_blocks[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30
PUBLIC struct fs_type fstype_table[NR_FS_TYPE];

PUBLIC int get_free_superblock()
{
	int sb_index = 0;
	for (sb_index = 0; sb_index < NR_SUPER_BLOCK; sb_index++)
	{
		if (super_blocks[sb_index].used == 0)
		{
			break;
		}
	}
	if (sb_index == NR_SUPER_BLOCK)
	{
		disp_str("there is no free superblock in array\n");
		return -1;
	}
	return sb_index;
}
#define MAX_DEV_PATH	16
int init_block_dev()
{
	for (int i = 0; i < 12; i++)
	{
		if (hd_infos[i].open_cnt > 0)
		{
			for (int j = 0; j < 16; j++)
			{
				if (hd_infos[i].part[j].size > 0)
				{
					int major = i,minor=j;
					register_device(MAKE_DEV(DEV_HD_BASE+i, minor), DEV_BLOCK_TYPE, &blk_file_ops);
				}
			}
		}
	}
	return 0;
}

// add by sundong 2023.5.19 
//在根文件系统下创建tty字符设备文件，设备文件分别是/dev/tty0、/dev/tty1、/dev/tty2
int init_char_dev()
{
	// struct super_block *sb = get_super_block(drive);
	//real_createdir(sb, "dev");
	// char ttypath[MAX_DEV_PATH] = {"/dev/tty0"};
	for (int i = 0; i < NR_CONSOLES; ++i)
	{
		// ttypath[strlen(ttypath) - 1] = '0' + i;
		// kern_vfs_mknod(ttypath, I_CHAR_SPECIAL|I_R|I_W, MAKE_DEV(DEV_CHAR_TTY, i));
		register_device(MAKE_DEV(DEV_CHAR_TTY, i), DEV_CHAR_TYPE, &tty_file_ops);
	}
	return 0;
}

// readpage to file address_space
// req. inode lock; file page cache list lock  
int generic_file_readpage(struct address_space* file_mapping, page* target) {
	struct vfs_inode* inode = file_mapping->host;
	struct inode_operations* ops = inode->i_op;
	if(!(ops && ops->get_block)) { // get block对于一般的文件系统必须实现！
		disp_str("error: no get_block op\n");
		return -1;
	}
	int size = inode->i_sb->sb_blocksize;
	int nr = PAGE_SIZE/size;
	if((PAGE_SIZE%size) || nr > MAX_BUF_PAGE){
		disp_str("block size must be 512/1024/2048/4096");
	}
	buf_head *bh = NULL;
	void* page_data = kmap(target);
	int block_start = target->pg_off * nr;
	for(int i = 0; i < nr; i++) {
		if(target->pg_buffer[i] == NULL){
			bh = (buf_head*)kern_kzalloc(sizeof(buf_head));
			bh->buffer = (void*) (page_data + i * size);
			initlock(&bh->lock, NULL);
			ops->get_block(inode, block_start + i, bh, 0);
			target->pg_buffer[i] = bh;
		}
	}
	
	for(int i = 0; i < nr; i++) {
		rw_buffer(DEV_READ, target->pg_buffer[i]);
	}
	// disp_str(" rd");
	// disp_int(target->pg_off);
	// disp_int(target->pg_buffer[0]->block);
	kunmap(target);
	return 0;
}

// writepage to file address_space
// req. inode lock; file page cache list lock 
int generic_file_writepage(struct address_space* file_mapping, page* target) {
	struct vfs_inode* inode = file_mapping->host;
	struct inode_operations* ops = inode->i_op;
	if(!(ops && ops->get_block)) { // get block对于一般的文件系统必须实现！
		disp_str("error: no get_block op\n");
		return -1;
	}
	int size = inode->i_sb->sb_blocksize;
	int nr = PAGE_SIZE/size;
	if((PAGE_SIZE%size) || nr > MAX_BUF_PAGE){
		disp_str("block size must be 512/1024/2048/4096");
	}
	buf_head *bh = NULL;
	void* page_data = kmap(target);
	int block_start = target->pg_off * nr;
	for(int i = 0; i < nr; i++) {
		if(target->pg_buffer[i] == NULL){
			bh = (buf_head*)kern_kzalloc(sizeof(buf_head));
			bh->buffer = (void*) (page_data + i * size);
			initlock(&bh->lock, NULL);
			ops->get_block(inode, block_start + i, bh, 1);
			target->pg_buffer[i] = bh;
		}
	}
	
	for(int i = 0; i < nr; i++) {
		rw_buffer(DEV_WRITE, target->pg_buffer[i]);
	}
	// disp_str(" wb");
	// disp_int(target->pg_off);
	// disp_int(target->pg_buffer[0]->block);
	kunmap(target);
	return 0;
}

// page cache read
// req. inode lock
int generic_file_read(struct file_desc* file, unsigned int count, char* buf) {
	mem_pages *page_cache = &file->fd_mapping->pages;
	u64 pos = file->fd_pos;
	u32 pgoff_start = pos >> PAGE_SHIFT;
	u64 end = min(pos + count, file->fd_dentry->d_inode->i_size);
	u32 pgoff_end = UPPER_BOUND_4K(end) >> PAGE_SHIFT;
	u32 page_offset = pos % PAGE_SIZE;
	int cnt = 0, len, total = end - LOWER_BOUND_4K(pos);
	page *_page = NULL;
	lock_or_yield(&page_cache->lock);
	for(u32 pgoff = pgoff_start; pgoff < pgoff_end; pgoff++)
	{
		_page = find_mem_page(page_cache, pgoff);
		if(_page == NULL) {
			_page = alloc_user_page(pgoff);
			generic_file_readpage(file->fd_mapping, _page);
			add_mem_page(page_cache, _page);
		}else {
			get_page(_page);
		}
		len = min(PAGE_SIZE, total) - page_offset;
		copy_from_page(_page, buf + cnt, len, page_offset);
		total -= (len + page_offset);
		cnt += len;
		page_offset = 0;
		put_page(_page);
	}
	release(&page_cache->lock);
	file->fd_pos += cnt;
	return cnt;
}

// page cache write
// req. inode lock
int generic_file_write(struct file_desc* file, unsigned int count, const char* buf) {
	mem_pages *page_cache = &file->fd_mapping->pages;
	u64 pos = file->fd_pos;
	u32 pgoff_start = pos >> PAGE_SHIFT;
	u64 end = pos + count;
	u32 pgoff_file_end = UPPER_BOUND_4K(file->fd_dentry->d_inode->i_size) >> PAGE_SHIFT;
	u32 pgoff_end = UPPER_BOUND_4K(end) >> PAGE_SHIFT;
	u32 page_offset = pos % PAGE_SIZE;
	int cnt = 0, len, total = end - LOWER_BOUND_4K(pos);
	page *_page = NULL;
	lock_or_yield(&page_cache->lock);
	for(u32 pgoff = pgoff_start; pgoff < pgoff_end; pgoff++)
	{
		_page = find_mem_page(page_cache, pgoff);
		if(_page == NULL) {
			_page = alloc_user_page(pgoff);
			if(pgoff < pgoff_file_end){
				generic_file_readpage(file->fd_mapping, _page);
			}else {
				zero_page(_page);
			}
			add_mem_page(page_cache, _page);
		}else {
			get_page(_page);
		}
		len = min(PAGE_SIZE, total) - page_offset;
		copy_to_page(_page, buf + cnt, len, page_offset);
		_page->dirty = 1;
		total -= (len + page_offset);
		cnt += len;
		page_offset = 0;
		put_page(_page);
	}
	release(&page_cache->lock);
	file->fd_pos += cnt;
	if(file->fd_pos > file->fd_dentry->d_inode->i_size) {
		file->fd_dentry->d_inode->i_size = file->fd_pos;
	}
	if(cnt != count) {
		disp_str("write cnt wrong?");
	}
	return cnt;
}

// req. inode lock
int generic_file_fsync(struct file_desc* file, int datasync)
{
	mem_pages *page_cache = &file->fd_mapping->pages;
	lock_or_yield(&page_cache->lock);
	writeback_mem_page(page_cache);
	release(&page_cache->lock);
	if(datasync){ // datasync表示只同步文件数据，不用同步inode等元数据
		return 0;
	}
	struct vfs_inode *inode = file->fd_dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	if(sb && sb->sb_op && sb->sb_op->write_inode) {
		sb->sb_op->write_inode(inode);
	}
	return 0;
}