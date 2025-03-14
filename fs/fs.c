#include <fs/devfs/devfs.h>
#include <minios/buffer.h>
#include <minios/hd.h>
#include <minios/memman.h>
#include <minios/mmap.h>
#include <minios/page.h>
#include <minios/tty.h>
#include <minios/vfs.h>
#include <minios/assert.h>
#include <klib/stddef.h>
#include <string.h>

struct super_block* super_blocks[NR_SUPER_BLOCK];
struct fs_type fstype_table[NR_FS_TYPE];

int get_free_superblock() {
    int sb_index = 0;
    for (sb_index = 0; sb_index < NR_SUPER_BLOCK; ++sb_index) {
        if (super_blocks[sb_index] == NULL) { break; }
    }
    if (sb_index == NR_SUPER_BLOCK) {
        kprintf("there is no free superblock in array\n");
        return -1;
    }
    return sb_index;
}

bool release_superblock(struct super_block* sb) {
    for (int i = 0; i < NR_SUPER_BLOCK; ++i) {
        if (super_blocks[i] == sb) {
            super_blocks[i] = NULL;
            kern_kfree(sb);
            return true;
        }
    }
    return false;
}

// mapping file page buffer
// return mapped block number
// req. inode lock; file page cache list lock
static int page_filemap(memory_page_t* target, address_space_t* file_mapping, int create) {
    buf_head* bh = NULL;
    struct inode* inode = file_mapping->host;
    struct inode_operations* ops = inode->i_op;
    int size = inode->i_sb->sb_blocksize;
    int nr = PGSIZE / size;
    int block_start = target->pg_off * nr;
    if (create == 0) {
        int nr_limit = ROUNDUP(inode->i_size, size) / size;
        nr = MIN(nr_limit - block_start, nr);
        if (nr == 0) { kprintf("error: over limit blk nr\n"); }
    }
    if ((PGSIZE % size) || nr > MAX_BUF_PAGE) { kprintf("block size must be 512/1024/2048/4096"); }
    if (!(ops && ops->get_block)) { // get block对于一般的文件系统必须实现！
        kprintf("error: no get_block op\n");
        return -1;
    }
    assert(target->user_va != NULL);
    void* page_data = target->user_va;
    int phy, last_phy, last;
    for (int i = 0; i < nr; ++i) {
        if (target->pg_buffer[i] == NULL) {
            phy = ops->get_block(inode, block_start + i, create);
            if (phy == -1) {
                kprintf("error: unmapped blk\n");
                kprintf("warning: data in file may lost\n");
                return i;
            }
            if (i && (phy == last_phy + i - last)) { continue; }
            bh = kern_kzalloc(sizeof(buf_head));
            bh->buffer = (void*)(page_data + i * size);
            spinlock_init(&bh->lock, NULL);
            map_bh(bh, inode->i_sb, phy);
            target->pg_buffer[i] = bh;
            last = i;
            last_phy = phy;
        }
    }
    assert((void*)target->pg_buffer[0]->buffer == page_data);
    return nr;
}

static void page_unmap_buffer(memory_page_t* target) {
    for (int i = 0; i < MAX_BUF_PAGE; ++i) {
        if (target->pg_buffer[i] != NULL) { kern_kfree(target->pg_buffer[i]); }
    }
    memset(target->pg_buffer, 0, sizeof(target->pg_buffer));
}

// readpage to file address_space
// req. inode lock; file page cache list lock
int generic_file_readpage(address_space_t* file_mapping, memory_page_t* target) {
    int nr = page_filemap(target, file_mapping, 0);
    int size = file_mapping->host->i_sb->sb_blocksize;
    int length = size;
    for (int i = nr - 1; i >= 0; i--) {
        if (target->pg_buffer[i] == NULL) {
            length += size;
            continue;
        }
        rw_buffer(HD_CMD_READ, target->pg_buffer[i], length);
    }
    page_unmap_buffer(target);
    return 0;
}

// writepage to file address_space
// req. inode lock; file page cache list lock
int generic_file_writepage(address_space_t* file_mapping, memory_page_t* target) {
    int nr = page_filemap(target, file_mapping, 1);
    int size = file_mapping->host->i_sb->sb_blocksize;
    int length = size;
    for (int i = nr - 1; i >= 0; i--) {
        if (target->pg_buffer[i] == NULL) {
            length += size;
            continue;
        }
        rw_buffer(HD_CMD_WRITE, target->pg_buffer[i], length);
    }
    page_unmap_buffer(target);
    return 0;
}

// page cache read
// req. inode lock
int generic_file_read(struct file_desc* file, unsigned int count, char* buf) {
    address_space_t* mapping = file->fd_mapping;
    u64 pos = file->fd_pos;
    u32 pgoff_start = phy_to_pfn(pos);
    u64 end = MIN(pos + count, file->fd_dentry->d_inode->i_size);
    u32 pgoff_end = phy_to_pfn(ROUNDUP(end, PGSIZE));
    u32 page_offset = pos % PGSIZE;
    int cnt = 0, len, total = end - ROUNDDOWN(pos, PGSIZE);
    memory_page_t* _page = NULL;
    spinlock_lock_or_yield(&mapping->lock);
    for (u32 pgoff = pgoff_start; pgoff < pgoff_end; ++pgoff) {
        _page = find_mem_page(mapping, pgoff);
        if (_page == NULL) {
            spinlock_release(&mapping->lock);
            _page = alloc_user_page(pgoff);
            generic_file_readpage(mapping, _page);
            spinlock_lock_or_yield(&mapping->lock);
            add_mem_page(mapping, _page);
        } else {
            get_page(_page);
        }
        len = MIN(PGSIZE, total) - page_offset;
        copy_from_page(_page, buf + cnt, len, page_offset);
        total -= (len + page_offset);
        cnt += len;
        page_offset = 0;
        put_page(_page);
    }
    spinlock_release(&mapping->lock);
    file->fd_pos += cnt;
    return cnt;
}

// page cache write
// req. inode lock
int generic_file_write(struct file_desc* file, unsigned int count, const char* buf) {
    address_space_t* mapping = file->fd_mapping;
    u64 pos = file->fd_pos;
    u32 pgoff_start = phy_to_pfn(pos);
    u64 end = pos + count;
    u32 pgoff_file_end = phy_to_pfn(ROUNDUP(file->fd_dentry->d_inode->i_size, PGSIZE));
    u32 pgoff_end = phy_to_pfn(ROUNDUP(end, PGSIZE));
    u32 page_offset = pos % PGSIZE;
    u32 cnt = 0, len;
    u64 total = end - ROUNDDOWN(pos, PGSIZE);
    memory_page_t* _page = NULL;
    spinlock_lock_or_yield(&mapping->lock);
    for (u32 pgoff = pgoff_start; pgoff < pgoff_end; ++pgoff) {
        _page = find_mem_page(mapping, pgoff);
        if (_page == NULL) {
            spinlock_release(&mapping->lock);
            _page = alloc_user_page(pgoff);
            if (pgoff < pgoff_file_end) {
                generic_file_readpage(mapping, _page);
            } else {
                zero_page(_page);
                if (page_filemap(_page, mapping, 1) > 0) {
                    page_unmap_buffer(_page);
                } else {
                    kprintf("error: no space for write\n");
                    put_page(_page);
                    break;
                }
            }
            spinlock_lock_or_yield(&mapping->lock);
            add_mem_page(mapping, _page);
        } else {
            get_page(_page);
        }
        len = MIN(PGSIZE, (int)total) - page_offset;
        copy_to_page(_page, buf + cnt, len, page_offset);
        _page->dirty = 1;
        total -= (len + page_offset);
        cnt += len;
        page_offset = 0;
        put_page(_page);
    }
    spinlock_release(&mapping->lock);
    file->fd_pos += cnt;
    if (file->fd_pos > file->fd_dentry->d_inode->i_size) {
        file->fd_dentry->d_inode->i_size = file->fd_pos;
    }
    if (cnt != count) { kprintf("write cnt wrong?"); }
    return cnt;
}

// req. inode lock
int generic_file_fsync(struct file_desc* file, int datasync) {
    address_space_t* mapping = file->fd_mapping;
    spinlock_lock_or_yield(&mapping->lock);
    pagecache_writeback(mapping);
    spinlock_release(&mapping->lock);
    if (datasync) { // datasync表示只同步文件数据，不用同步inode等元数据
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
