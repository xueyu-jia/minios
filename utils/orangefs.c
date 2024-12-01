#include <stddef.h>
#include <stdlib.h>
#include <string.h>
// #include <fuse_log.h>
#include "orangefs.h"
#include "orangefs_disk.h"

static struct orange_sb superblock;
static list_head inode_list;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;

int orangefs_fillsuper() {
  disk_read(1, 0, sizeof(struct orange_sb), &superblock);
  list_init(&inode_list);
  if (superblock.magic != ORANGE_MAGIC) {
    return -1;
  }
  return 0;
}

// free inode cache, mutex lock required
static void orangefs_clear_inodecache(struct inode_cache *ic) {
  free(ic->iptr);
  list_remove(&ic->ilist);
  free(ic);
}

void orangefs_free() {
  struct inode_cache *ic;
  pthread_mutex_lock(&cache_lock);
  while (!list_empty(&inode_list)) {
    ic = list_front(&inode_list, struct inode_cache, ilist);
    orangefs_clear_inodecache(ic);
  }
  pthread_mutex_unlock(&cache_lock);
  disk_close();
}

static void orangefs_readinode(int ino, struct orange_inode *inode) {
  disk_read(2 + superblock.nr_imap_blocks + superblock.nr_smap_blocks,
            (ino - 1) * ORANGE_INODE_SIZE, ORANGE_INODE_SIZE, (void *)inode);
}

void orangefs_syncinode(int ino, struct orange_inode *inode) {
  disk_write(2 + superblock.nr_imap_blocks + superblock.nr_smap_blocks,
             (ino - 1) * ORANGE_INODE_SIZE, ORANGE_INODE_SIZE, (void *)inode);
}

static int orangefs_alloc_bitmap(int blk_base, int blk_nr, int cnt,
                                 int *pindex) {
  int index = -1, i, j, total = cnt;
  u8 c, *buf = malloc(ORANGE_BLK_SIZE);
  for (int blk = 0; blk < blk_nr; blk++) {
    disk_read(blk_base + blk, 0, ORANGE_BLK_SIZE, buf);
    for (i = 0; i < ORANGE_BLK_SIZE; i++) {
      if (buf[i] != 0xFF) {
        c = buf[i];
        j = 0;
        if (index == -1) {
          while ((c >> j) & 1) {
            j++;
          }
          index = ((blk * ORANGE_BLK_SIZE + i) << 3) | j;
        }
        while (cnt && j < 8 && (((c >> j) & 1) == 0)) {
          c |= (1 << j);
          cnt--;
          j++;
        }
        buf[i] = c;
        if (cnt == 0 || j < 8) {
          disk_write(blk_base + blk, 0, ORANGE_BLK_SIZE, buf);
          goto finish;
        }
      } else if (index != -1) {
        disk_write(blk_base + blk, 0, ORANGE_BLK_SIZE, buf);
        goto finish;
      }
    }
    disk_write(blk_base + blk, 0, ORANGE_BLK_SIZE, buf);
  }
finish:
  *pindex = index;
  free(buf);
  return total - cnt;
}

// free bitmap in [start, end)
static void orangefs_free_bitmap(int blk_base, int start, int end) {
  if (start >= end) {
    return;
  }
  int start_byte = start / 8, end_byte = (end / 8) + 1;
  int len = end_byte - start_byte;
  u8 *buf = malloc(end_byte - start_byte);
  disk_read(blk_base, start_byte, len, buf);
  buf[0] &= ((1 << (start & 7)) - 1);
  memset(buf + 1, 0, len - 2);
  buf[len - 1] &= (~((1 << (end & 7)) - 1));
  disk_write(blk_base, start_byte, len, buf);
  free(buf);
}

int orangefs_allocinode() {  // return inode_nr
  int index;
  if (orangefs_alloc_bitmap(2, superblock.nr_imap_blocks, 1, &index) == 1) {
    // fuse_log(FUSE_LOG_DEBUG, "alloc inode:%d\n", index + 1);
    return index;
  }
  return -1;
}

void orangefs_allocsector(struct orange_inode *pin) {
  int index;
  int nr_block = max(1, min(superblock.nr_blocks / 4, ORANGE_FILE_BLK));
  pin->i_nr_blocks =
      orangefs_alloc_bitmap(2 + superblock.nr_imap_blocks,
                            superblock.nr_smap_blocks, nr_block, &index);
  pin->i_start_block = index + superblock.n_1st_block;
  // fuse_log(FUSE_LOG_DEBUG, "alloc sec:[%d,%d)\n", pin->i_start_block,
  // pin->i_nr_blocks + pin->i_start_block);
}

void orangefs_iforget_may_drop(int ino, struct orange_inode *inode) {
  if (inode->deleted) {
    orangefs_free_bitmap(2, ino, ino + 1);
    orangefs_free_bitmap(2 + superblock.nr_imap_blocks, inode->i_start_block,
                         inode->i_start_block + inode->i_nr_blocks);
  }
  pthread_mutex_lock(&cache_lock);
  struct inode_cache *ic = NULL, *target = NULL;
  list_for_each(&inode_list, ic, ilist) {
    if (ic->iptr == inode) {
      target = ic;
    }
  }
  if (target) {
    orangefs_clear_inodecache(target);
  }
  pthread_mutex_unlock(&cache_lock);
}

struct orange_inode *orangefs_iget(int ino) {
  struct orange_inode *inode = NULL;
  struct inode_cache *ic = NULL;
  if (ino <= 0) {
    return NULL;
  }
  pthread_mutex_lock(&cache_lock);
  list_for_each(&inode_list, ic, ilist) {
    if (ic->ino == ino) {
      inode = ic->iptr;
      break;
    }
  }
  if (NULL == inode) {
    inode = malloc(sizeof(struct orange_inode));
    memset(inode, 0, sizeof(struct orange_inode));
    orangefs_readinode(ino, inode);
    list_for_each(&inode_list, ic, ilist) {
      if (ic->ino >= ino) {
        break;
      }
    }
    struct inode_cache *new_ic = malloc(sizeof(struct inode_cache));
    list_init(&new_ic->ilist);
    new_ic->ino = ino;
    _list_insert(&ic->ilist, ic->ilist.prev, &ic->ilist);
  }
  pthread_mutex_unlock(&cache_lock);
  return inode;
}

// 0 for not found
int orangefs_find(int parent, const char *name, int *rt_pos) {
  struct orange_inode *dir = orangefs_iget(parent);
  struct orange_direntry dirent;
  memset(&dirent, 0, ORANGE_DIRENT_SIZE);
  int p = 0;
  for (int blk = dir->i_start_block;
       blk < dir->i_start_block + dir->i_nr_blocks; blk++) {
    for (int pos = 0; pos < ORANGE_BLK_SIZE && p + pos < dir->i_size;
         pos += ORANGE_DIRENT_SIZE) {
      disk_read(blk, pos, ORANGE_DIRENT_SIZE, &dirent);
      if (dirent.inode_nr == 0) {
        continue;
      }
      if (strncmp(dirent.name, name, MAX_FILENAME_LEN) == 0) {
        if (rt_pos) {
          *rt_pos = p + pos;
        }
        return dirent.inode_nr;
      }
    }
    p += ORANGE_BLK_SIZE;
  }
  return 0;
}

// 0 for empty
int orangefs_dir_empty(struct orange_inode *dir) {
  struct orange_direntry dirent;
  memset(&dirent, 0, ORANGE_DIRENT_SIZE);
  int p = 0, pos;
  for (int blk = dir->i_start_block;
       blk < dir->i_start_block + dir->i_nr_blocks; blk++) {
    for (pos = 0; pos < ORANGE_BLK_SIZE; pos += ORANGE_DIRENT_SIZE) {
      disk_read(blk, pos, ORANGE_DIRENT_SIZE, &dirent);
      if (dirent.inode_nr == 0) {
        continue;
      }
      if ((strcmp(dirent.name, ".") == 0) || (strcmp(dirent.name, "..") == 0)) {
        continue;
      }
      return -1;
    }
    if (p + pos >= dir->i_size) {
      goto out;
    }
    p += ORANGE_BLK_SIZE;
  }
out:
  return 0;
}

void orangefs_write_direntry(struct orange_inode *dir, int pos,
                             const char *name, int ino) {
  struct orange_direntry dirent;
  memset(&dirent, 0, ORANGE_DIRENT_SIZE);
  strncpy(dirent.name, name, MAX_FILENAME_LEN);
  dirent.inode_nr = ino;
  disk_write(dir->i_start_block, pos, ORANGE_DIRENT_SIZE, &dirent);
}

int orangefs_add_direntry(int parent, const char *name, int ino) {
  struct orange_inode *dir = orangefs_iget(parent);
  struct orange_direntry dirent;
  memset(&dirent, 0, ORANGE_DIRENT_SIZE);
  int blk, pos, p = 0;
  if (dir->i_nr_blocks == 0) {
    orangefs_allocsector(dir);
    orangefs_syncinode((int)parent, dir);
  }
  // fuse_log(FUSE_LOG_DEBUG, "dir %d(%d)\n", parent, dir->i_size);
  for (blk = dir->i_start_block; blk < dir->i_start_block + dir->i_nr_blocks;
       blk++) {
    for (pos = 0; pos < ORANGE_BLK_SIZE; pos += ORANGE_DIRENT_SIZE) {
      disk_read(blk, pos, ORANGE_DIRENT_SIZE, &dirent);
      // fuse_log(FUSE_LOG_DEBUG, "dir %d.%d: %d\n", parent, p + pos,
      // dirent.inode_nr);
      if (p + pos >= dir->i_size) {
        dir->i_size = p + pos + ORANGE_DIRENT_SIZE;
        orangefs_syncinode(parent, dir);
        // fuse_log(FUSE_LOG_DEBUG, "inc dir %d size to %d\n", parent,
        // dir->i_size);
        goto findfree;
      }
      if (dirent.inode_nr == 0) {
        goto findfree;
      }
    }
    p += ORANGE_BLK_SIZE;
  }
  if (blk == dir->i_start_block + dir->i_nr_blocks) {
    return -1;
  }
findfree:
  orangefs_write_direntry(dir, p + pos, name, ino);
  return 0;
}

int orangefs_stat(int ino, struct stat *buf) {
  struct orange_inode *pin = orangefs_iget(ino);
  if (pin == NULL) {
    return -1;
  }
  buf->st_ino = ino;
  buf->st_size = pin->i_size;
  buf->st_nlink = (ino == 1) ? 2 : 1;
  buf->st_blksize = ORANGE_BLK_SIZE;
  buf->st_blocks = pin->i_nr_blocks;
  u32 m = I_TYPE_MASK & pin->i_mode;
  buf->st_mode =
      0777 | ((m == I_DIRECTORY) ? S_IFDIR : ((m == I_REGULAR) ? S_IFREG : 0));
  return 0;
}
