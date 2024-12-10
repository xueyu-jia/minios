#include <kernel/buffer.h>
#include <kernel/devfs.h>
#include <kernel/hd.h>
#include <kernel/memman.h>
#include <kernel/mmap.h>
#include <kernel/tty.h>
#include <kernel/vfs.h>
#include <klib/assert.h>
#include <klib/string.h>

PUBLIC struct super_block* super_blocks[NR_SUPER_BLOCK];
PUBLIC struct fs_type fstype_table[NR_FS_TYPE];

PUBLIC int get_free_superblock() {
  int sb_index = 0;
  for (sb_index = 0; sb_index < NR_SUPER_BLOCK; sb_index++) {
    if (super_blocks[sb_index] == NULL) {
      break;
    }
  }
  if (sb_index == NR_SUPER_BLOCK) {
    disp_str("there is no free superblock in array\n");
    return -1;
  }
  return sb_index;
}
#define MAX_DEV_PATH 16
int init_block_dev() {
  for (int i = 0; i < 12; i++) {
    if (hd_infos[i].open_cnt > 0) {
      for (int j = 0; j < 16; j++) {
        if (hd_infos[i].part[j].size > 0) {
          int major = i, minor = j;
          UNUSED(major);
          register_device(MAKE_DEV(DEV_HD_BASE + i, minor), DEV_BLOCK_TYPE,
                          &blk_file_ops);
        }
      }
    }
  }
  return 0;
}

// add by sundong 2023.5.19
// 在根文件系统下创建tty字符设备文件，设备文件分别是/dev/tty0、/dev/tty1、/dev/tty2
int init_char_dev() {
  // struct super_block *sb = get_super_block(drive);
  // real_mkdir(sb, "dev");
  // char ttypath[MAX_DEV_PATH] = {"/dev/tty0"};
  for (int i = 0; i < NR_CONSOLES; ++i) {
    // ttypath[strlen(ttypath) - 1] = '0' + i;
    // kern_vfs_mknod(ttypath, I_CHAR_SPECIAL|I_R|I_W, MAKE_DEV(DEV_CHAR_TTY,
    // i));
    register_device(MAKE_DEV(DEV_CHAR_TTY, i), DEV_CHAR_TYPE, &tty_file_ops);
  }
  return 0;
}

// mapping file page buffer
// return mapped block number
// req. inode lock; file page cache list lock
PRIVATE int page_filemap(page* target, struct address_space* file_mapping,
                         int create) {
  buf_head* bh = NULL;
  struct inode* inode = file_mapping->host;
  struct inode_operations* ops = inode->i_op;
  int size = inode->i_sb->sb_blocksize;
  int nr = PAGE_SIZE / size;
  int block_start = target->pg_off * nr;
  if (create == 0) {
    int nr_limit = UPPER_BOUND(inode->i_size, size) / size;
    nr = min(nr_limit - block_start, nr);
    if (nr == 0) {
      disp_str("error: over limit blk nr\n");
    }
  }
  if ((PAGE_SIZE % size) || nr > MAX_BUF_PAGE) {
    disp_str("block size must be 512/1024/2048/4096");
  }
  if (!(ops && ops->get_block)) {  // get block对于一般的文件系统必须实现！
    disp_str("error: no get_block op\n");
    return -1;
  }
  void* page_data = (void*)kpage_lin(target);
  int phy, last_phy, last;
  for (int i = 0; i < nr; i++) {
    if (target->pg_buffer[i] == NULL) {
      phy = ops->get_block(inode, block_start + i, create);
      if (phy == -1) {
        disp_str("error: unmapped blk\n");
        disp_str("warning: data in file may lost\n");
        return i;
      }
      if (i && (phy == last_phy + i - last)) {
        continue;
      }
      bh = (buf_head*)kern_kzalloc(sizeof(buf_head));
      bh->buffer = (void*)(page_data + i * size);
      initlock(&bh->lock, NULL);
      map_bh(bh, inode->i_sb, phy);
      target->pg_buffer[i] = bh;
      last = i;
      last_phy = phy;
    }
  }
  assert((void*)target->pg_buffer[0]->buffer == page_data);
  return nr;
}

PRIVATE void page_unmap_buffer(page* target) {
  for (int i = 0; i < MAX_BUF_PAGE; i++) {
    if (target->pg_buffer[i] != NULL) {
      kern_kfree((u32)target->pg_buffer[i]);
    }
  }
  memset(target->pg_buffer, 0, sizeof(target->pg_buffer));
}

// readpage to file address_space
// req. inode lock; file page cache list lock
int generic_file_readpage(struct address_space* file_mapping, page* target) {
  int nr = page_filemap(target, file_mapping, 0);
  int size = file_mapping->host->i_sb->sb_blocksize;
  int length = size;
  for (int i = nr - 1; i >= 0; i--) {
    if (target->pg_buffer[i] == NULL) {
      length += size;
      continue;
    }
    rw_buffer(DEV_READ, target->pg_buffer[i], length);
  }
  page_unmap_buffer(target);
  // disp_str("\nrp:");
  // disp_int(page_to_pfn(target));
  // disp_int(target->pg_buffer[0]->block);
  // kunmap(target);
  return 0;
}

// writepage to file address_space
// req. inode lock; file page cache list lock
int generic_file_writepage(struct address_space* file_mapping, page* target) {
  int nr = page_filemap(target, file_mapping, 1);
  int size = file_mapping->host->i_sb->sb_blocksize;
  int length = size;
  for (int i = nr - 1; i >= 0; i--) {
    if (target->pg_buffer[i] == NULL) {
      length += size;
      continue;
    }
    rw_buffer(DEV_WRITE, target->pg_buffer[i], length);
  }
  page_unmap_buffer(target);
  // disp_str("\nwp:");
  // disp_int(page_to_pfn(target));
  // kunmap(target);
  return 0;
}

// page cache read
// req. inode lock
int generic_file_read(struct file_desc* file, unsigned int count, char* buf) {
  struct address_space* mapping = file->fd_mapping;
  u64 pos = file->fd_pos;
  u32 pgoff_start = pos >> PAGE_SHIFT;
  u64 end = min(pos + count, file->fd_dentry->d_inode->i_size);
  u32 pgoff_end = UPPER_BOUND_4K(end) >> PAGE_SHIFT;
  u32 page_offset = pos % PAGE_SIZE;
  int cnt = 0, len, total = end - LOWER_BOUND_4K(pos);
  page* _page = NULL;
  lock_or_yield(&mapping->lock);
  for (u32 pgoff = pgoff_start; pgoff < pgoff_end; pgoff++) {
    _page = find_mem_page(mapping, pgoff);
    if (_page == NULL) {
      release(&mapping->lock);
      _page = alloc_user_page(pgoff);
      generic_file_readpage(mapping, _page);
      lock_or_yield(&mapping->lock);
      add_mem_page(mapping, _page);
    } else {
      get_page(_page);
    }
    len = min(PAGE_SIZE, total) - page_offset;
    copy_from_page(_page, buf + cnt, len, page_offset);
    // disp_str(" rd:");
    // disp_int(len);
    total -= (len + page_offset);
    cnt += len;
    page_offset = 0;
    put_page(_page);
  }
  release(&mapping->lock);
  file->fd_pos += cnt;
  return cnt;
}

// page cache write
// req. inode lock
int generic_file_write(struct file_desc* file, unsigned int count,
                       const char* buf) {
  struct address_space* mapping = file->fd_mapping;
  u64 pos = file->fd_pos;
  u32 pgoff_start = pos >> PAGE_SHIFT;
  u64 end = pos + count;
  u32 pgoff_file_end =
      UPPER_BOUND_4K(file->fd_dentry->d_inode->i_size) >> PAGE_SHIFT;
  u32 pgoff_end = UPPER_BOUND_4K(end) >> PAGE_SHIFT;
  u32 page_offset = pos % PAGE_SIZE;
  u32 cnt = 0, len;
  u64 total = end - LOWER_BOUND_4K(pos);
  page* _page = NULL;
  lock_or_yield(&mapping->lock);
  for (u32 pgoff = pgoff_start; pgoff < pgoff_end; pgoff++) {
    _page = find_mem_page(mapping, pgoff);
    if (_page == NULL) {
      release(&mapping->lock);
      _page = alloc_user_page(pgoff);
      if (pgoff < pgoff_file_end) {
        generic_file_readpage(mapping, _page);
      } else {
        zero_page(_page);
        if (page_filemap(_page, mapping, 1) > 0) {
          page_unmap_buffer(_page);
        } else {
          disp_str("error: no space for write\n");
          put_page(_page);
          break;
        }
      }
      lock_or_yield(&mapping->lock);
      add_mem_page(mapping, _page);
    } else {
      get_page(_page);
    }
    len = min(PAGE_SIZE, (int)total) - page_offset;
    copy_to_page(_page, buf + cnt, len, page_offset);
    _page->dirty = 1;
    total -= (len + page_offset);
    cnt += len;
    page_offset = 0;
    put_page(_page);
  }
  release(&mapping->lock);
  file->fd_pos += cnt;
  if (file->fd_pos > file->fd_dentry->d_inode->i_size) {
    file->fd_dentry->d_inode->i_size = file->fd_pos;
  }
  if (cnt != count) {
    disp_str("write cnt wrong?");
  }
  return cnt;
}

// req. inode lock
int generic_file_fsync(struct file_desc* file, int datasync) {
  struct address_space* mapping = file->fd_mapping;
  lock_or_yield(&mapping->lock);
  pagecache_writeback(mapping);
  release(&mapping->lock);
  if (datasync) {  // datasync表示只同步文件数据，不用同步inode等元数据
    return 0;
  }
  struct inode* inode = file->fd_dentry->d_inode;
  vfs_sync_inode(inode);
  return 0;
}

void generic_query_size_info(struct fs_size_info* info) {
  info->sb_size = 0;
  info->inode_size = 0;
}
