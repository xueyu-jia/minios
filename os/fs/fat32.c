#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/const.h>
#include <kernel/fat32.h>
#include <kernel/memman.h>
#include <kernel/time.h>
#include <kernel/type.h>
#include <kernel/vfs.h>
#include <klib/compiler.h>
#include <klib/string.h>

PRIVATE void uni2char(u16* uni_name, char* norm_name, int max_len) {
  u16 c;
  for (int i = 0; (c = *(uni_name++)) && i < max_len; i++) {
    *(norm_name++) = c & 0xFF;
  }
  *norm_name = 0;
}

PRIVATE void char2uni(u16* uni_name, const char* norm_name, int max_len) {
  char c;
  for (int i = 0; (c = *(norm_name++)) && i < max_len; i++) {
    *(uni_name++) = c;
  }
  *uni_name = 0;
}

PRIVATE u8 fat_chksum(const char* shortname) {
  u8 sum = 0;
  int i;
  for (sum = 0, i = 0; i < 11; i++) {
    sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + shortname[i];
    // 两部分的name实际是连续的，所以get entry 中传入de->name没问题
  }
  return sum;
}
/*
根据FAT文件系统文档:
短文件名中，
 所有大写字母和数字，以及下面的特殊字符合法
 $ % ' - _ @ ~ ` ! ( ) { } ^ # &
长文件名中，除了短文件名的合法字符外，
" "(space)和"."(period) (在短文件名中忽略)以及 + , ; = [ ]
(在短文件名中替换为"_")也合法

其余的字符，根据ASCII表，即space之前的控制字符以及: " * / : < > ? \ |
为非法字符(bad char)
*/
// 判断c是否是非法字符
PRIVATE inline int fat_bad_char(char c) {
  return ((c < 0x20) || (strchr("\"*/\\:<>?|", c) != NULL));
}

PRIVATE inline int fat_skip_char(char c) { return ((c == ' ') || (c == '.')); }

// 判断c是否需要在短文件名替换为"_"
PRIVATE inline int fat_replace_char(char c) {
  return (strchr("+,;=[]", c) != NULL);
}

PRIVATE char fat_shortname_char(char c) {
  if (c >= 'a' && c <= 'z') {
    c = c - 'a' + 'A';
  }
  if (fat_replace_char(c)) {
    c = '_';
  }
  return c;
}

PRIVATE int fat_badname(const char* name) {
  if (NULL == name) return 1;
  for (const char* s = name; *s; s++) {
    if (fat_bad_char(*s)) {
      return 1;
    }
  }
  return 0;
}

PRIVATE void fat_update_datetime(u32 timestamp, u16* date, u16* time,
                                 u8* centi_sec) {
  struct tm time_s;
  gmtime(timestamp, &time_s);
  if (date != NULL) {
    *date = ((time_s.tm_year + 1900 - 1980) << 9) | ((time_s.tm_mon + 1) << 5) |
            (time_s.tm_mday);
  }
  if (time != NULL) {
    *time = ((time_s.tm_hour) << 11) | ((time_s.tm_min) << 5) |
            (time_s.tm_sec >> 1);
  }
  if (centi_sec != NULL) {
    *centi_sec = (time_s.tm_sec & 1) * 100;
  }
}

PRIVATE u32 fat_read_datetime(u16 date, u16 time, u8 centi_sec) {
  struct tm time_s;
  time_s.__tm_gmtoff = 0;
  time_s.tm_year = (date >> 9) + 1980 - 1900;
  time_s.tm_mon = ((date >> 5) & 0xF) - 1;
  time_s.tm_mday = ((date) & 0x1F);
  time_s.tm_hour = ((time >> 11) & 0x1F);
  time_s.tm_min = ((time >> 5) & 0x2F);
  time_s.tm_sec = ((time & 0x1F) << 1) + (centi_sec) / 100;
  return mktime(&time_s);
}

PRIVATE inline struct fat_info* alloc_fat_info(int cluster_start) {
  struct fat_info* ent =
      (struct fat_info*)kern_kmalloc(sizeof(struct fat_info));
  ent->cluster_start = cluster_start;
  ent->length = 1;
  ent->next = NULL;
  return ent;
}

PRIVATE inline void free_fat_info(struct fat_info* info) {
  kern_kfree((u32)info);
}

// if *bh not NULL, brelse origin blk
// getblk by fat logic block(may 512 bytes...), return buf pointer and update bh
PRIVATE char* fat_bread(struct super_block* sb, int block, buf_head** bh) {
  buf_head* b = NULL;
  u32 dev = sb->sb_dev;
  int block_per_4k = (BLOCK_SIZE / sb->sb_blocksize);
  u32 block_4k = block / block_per_4k;
  if ((*bh) && (*bh)->block == block_4k && (*bh)->dev == dev) {
    b = *bh;  // 避免没必要的bread
  } else {
    b = bread(dev, block_4k);
    if (!b) return NULL;
    if (*bh) {
      brelse(*bh);
    }
    *bh = b;
  }
  return ((char*)b->buffer) + ((block & (block_per_4k - 1)) * sb->sb_blocksize);
}

// value == -1 for read
PRIVATE int read_write_fat(struct super_block* sb, int cluster, int value) {
  int dev = sb->sb_dev, next;
  if (cluster > FAT_SB(sb)->max_cluster) {
    return 0;
  }
  int fat_offset = cluster * 4;
  // only support FAT32, other implements should judge FAT_SB(sb)->fat_bit
  int fat_block = FAT_SB(sb)->fat_start_block + (fat_offset / sb->sb_blocksize);
  struct buf_head *bh = NULL, *bh2 = NULL;
  char* buf = fat_bread(sb, fat_block, &bh);
  next = ((u32*)(buf))[(fat_offset & (sb->sb_blocksize - 1)) >> 2];
  if (next >= 0xffffff7) next = FAT_END;
  if (value != -1) {  // write
    ((u32*)(buf))[(fat_offset & (sb->sb_blocksize - 1)) >> 2] = value;
    mark_buff_dirty(bh);
    // fat copy
    for (int fat = 1; fat < FAT_SB(sb)->fat_num; fat++) {
      fat_block = FAT_SB(sb)->fat_start_block + (fat * FAT_SB(sb)->fat_size) +
                  (fat_offset / sb->sb_blocksize);
      fat_bread(sb, fat_block, &bh2);
      memcpy(bh2->buffer, bh->buffer, num_4K);
      mark_buff_dirty(bh2);
      brelse(bh2);
    }
  }
  brelse(bh);
  return next;
}

PRIVATE int add_new_cluster(struct super_block* sb, int tail) {
  if (FAT_SB(sb)->fsinfo.cluster_free_count <= 0) {
    return -1;
  }
  acquire(&sb->lock);
  int i, clus, next_free = FAT_SB(sb)->fsinfo.cluster_next_free,
               total = FAT_SB(sb)->max_cluster - 1;
  buf_head* bh = NULL;
  for (i = 0; i < total;
       i++) {  // we must check before use,(linux dont use fsinfo.nextfree)
    clus = 2 + (next_free - 2 + i) % total;
    if (read_write_fat(sb, clus, -1) == 0) break;
  }
  if (i >= total) {
    FAT_SB(sb)->fsinfo.cluster_free_count = 0;
    disp_str("error: no space");
    release(&sb->lock);
    return -1;
  }
  read_write_fat(sb, clus, FAT_END);
  FAT_SB(sb)->fsinfo.cluster_free_count--;
  // update fsinfo
  for (i = 2; i <= total + 1; i++) {
    if (read_write_fat(sb, i, -1) == 0) break;
  }
  FAT_SB(sb)->fsinfo.cluster_next_free = i;
  struct fat_fsinfo* info_buf =
      (struct fat_fsinfo*)(fat_bread(sb, FAT_SB(sb)->fsinfo_block, &bh) +
                           FS_INFO_SECTOR_OFFSET);
  memcpy(info_buf, &FAT_SB(sb)->fsinfo, sizeof(struct fat_fsinfo));
  mark_buff_dirty(bh);
  brelse(bh);
  if (tail) {
    read_write_fat(sb, tail, clus);
  }
  release(&sb->lock);
  return clus;
}

PRIVATE int fill_fat_info(struct super_block* sb, int cluster_start,
                          struct fat_info** info) {
  struct fat_info* fat_head = alloc_fat_info(cluster_start);
  struct fat_info *fat_tail = fat_head, *fat_ent = NULL;
  int cluster = cluster_start, next, count = 0;
  *info = fat_head;
  if (!cluster) {
    return 0;  // according to FAT doc, cluster in entry will be 0 for empty
               // file;
  }
  acquire(&sb->lock);
  while (cluster != FAT_END) {
    next = read_write_fat(sb, cluster, -1);
    if (next == cluster + 1) {
      fat_tail->length++;
    } else if (next != FAT_END) {
      fat_ent = alloc_fat_info(next);
      fat_tail->next = fat_ent;
      fat_tail = fat_ent;
    }
    cluster = next;
    count++;
  }
  release(&sb->lock);
  return count;
}

PRIVATE int fat_get_cluster(struct inode* inode, int cluster, int new_space) {
  struct super_block* sb = inode->i_sb;
  struct fat_sb_info* sbi = FAT_SB(sb);
  int clus_skip = cluster;
  int clus, last = 0;
  struct fat_info* info = FAT_INODE(inode)->fat_info;
  int new = 0;
  if (!info->cluster_start) {
    if (!new_space) {
      return -1;
    }
    clus = add_new_cluster(sb, last);
    info->cluster_start = clus;
    info->length = 1;
    new = 1;
  }
  while (clus_skip && info) {
    if (clus_skip < info->length) break;
    if (!info->next && new_space) {
      last = info->cluster_start + info->length - 1;
      clus = add_new_cluster(sb, last);
      if (!last) {
        info->cluster_start = clus;
        continue;
      }
      if (clus == last + 1) {
        info->length++;
        continue;
      } else {  // new alloc cluster not consecutive
        info->next = alloc_fat_info(clus);
      }
      // new = 1;
    }
    clus_skip -= info->length;
    info = info->next;
  }
  if (!info) {
    return -1;
  }
  // if(new){
  // 	fat32_sync_inode(inode);
  // }
  return info->cluster_start + clus_skip;
}

PUBLIC int fat32_get_block(struct inode* inode, u32 iblock, int create) {
  struct fat32_sb_info* sbi = FAT_SB(inode->i_sb);
  int clus = fat_get_cluster(inode, iblock / sbi->cluster_block, create);
  if (clus == -1) {
    return -1;
  }
  return sbi->data_start_block + (clus - 2) * sbi->cluster_block +
         (iblock % sbi->cluster_block);
}

// return entry offset by 32 bytes
PRIVATE int fat_entry_offset_by_ino(struct inode* dir, u32 ino) {
  struct super_block* sb = dir->i_sb;
  struct fat32_sb_info* sbi = FAT_SB(sb);
  int entry_block = sb->sb_blocksize / FAT_ENTRY_SIZE;
  int entry = ino % entry_block;
  int block_offset = (ino / entry_block) - sbi->data_start_block;
  int cluster = block_offset / sbi->cluster_block + 2;
  entry += (block_offset % sbi->cluster_block) *
           entry_block;  // 计算最后一簇的偏移量
  int start = 0, found = 0;
  struct fat_info* info = FAT_INODE(dir)->fat_info;
  for (auto info = FAT_INODE(dir)->fat_info; info; info = info->next) {
    if (cluster >= info->cluster_start &&
        cluster < info->cluster_start + info->length) {
      found = 1;
      start +=
          (sbi->cluster_block * (cluster - info->cluster_start)) * entry_block;
      break;
    }
    start += (sbi->cluster_block * info->length) * entry_block;
  }
  if (!found) {
    return -1;
  }
  return start + entry;
}

PRIVATE inline u32 fat_ino(struct inode* dir, int entry) {
  struct super_block* sb = dir->i_sb;
  int entry_block = sb->sb_blocksize / FAT_ENTRY_SIZE;
  int block = fat32_get_block(dir, (entry / entry_block), 0);
  return block * entry_block + entry % entry_block;
}

PRIVATE char* fat_get_data(struct inode* inode, int off, buf_head** bh,
                           int alloc_new) {
  struct super_block* sb = inode->i_sb;
  int clus_block = fat32_get_block(inode, off / sb->sb_blocksize, alloc_new);
  if (clus_block == -1) {
    return NULL;
  }
  return fat_bread(sb, clus_block, bh) + (off & (sb->sb_blocksize - 1));
}

PRIVATE inline struct fat_dir_slot* fat_get_slot(struct inode* dir, int order,
                                                 buf_head** bh, int alloc_new) {
  return (struct fat_dir_slot*)fat_get_data(dir, (order)*FAT_ENTRY_SIZE, bh,
                                            alloc_new);
}

// get next fat short entry from *%start in inode %dir,
//   long dir full name stored in %full_name and %bh updated
PRIVATE struct fat_dir_entry* fat_get_entry(struct inode* dir, int* start,
                                            buf_head** bh, char* full_name) {
  if (!start) {
    return NULL;
  }
  struct fat_dir_entry* de = NULL;
  struct fat_dir_slot* ds;
  int n_slot = 0, offset, is_long = 0;
  u8 chksum;
  u16 uni_name[256];
  for (; (u64)(*start) * FAT_ENTRY_SIZE < dir->i_size; (*start)++) {
    ds = fat_get_slot(dir, *start, bh, 0);
    if (!ds) break;
    if (ds->order == 0) {
      break;
    }
    if (ds->order == DIR_DELETE) {
      is_long = 0;
      continue;
    }
    if (ds->attr == ATTR_LNAME) {
      if (ds->order & DIR_LDIR_END) {
        n_slot = ds->order & (~DIR_LDIR_END);
        chksum = ds->checksum;
        is_long = 1;
      }
      if ((n_slot != (ds->order & (~DIR_LDIR_END))) || chksum != ds->checksum) {
        disp_str("error: dismatch slot");
        is_long = 0;
        continue;  // ignore invalid slot
      }
      offset = (n_slot - 1) * 13;
      memcpy(&uni_name[offset], ds->name0_4, 10);
      offset += 5;
      memcpy(&uni_name[offset], ds->name5_10, 12);
      offset += 6;
      memcpy(&uni_name[offset], ds->name11_12, 4);
      offset += 2;
      if (ds->order & DIR_LDIR_END) {
        uni_name[offset] = 0;
      }
      n_slot--;
    } else if (!(ds->attr & ATTR_VOL)) {  // skip volume
      // check sum
      de = (struct fat_dir_entry*)ds;
      if (is_long) {
        u8 sum = fat_chksum(de->name);
        if (sum != chksum) {
          disp_str("error: chksum wrong");
          is_long = 0;
          continue;
        }
      }
      if (is_long) {
        uni2char(uni_name, full_name, 256);
      } else {
        if (!strncmp(de->name, FAT_DOT, 11)) {
          strcpy(full_name, ".");
        } else if (!strncmp(de->name, FAT_DOTDOT, 11)) {
          strcpy(full_name, "..");
        } else {
          memcpy(full_name, de->name, 8);
          full_name[8] = '.';
          memcpy(full_name + 9, de->ext, 3);
          full_name[12] = 0;
        }
      }
      break;
    }
  }
  return de;
}

PRIVATE int fat_find_free(struct inode* dir, int num) {
  buf_head* bh = NULL;
  struct fat_dir_slot* ds;
  int start, count = 0, res = -1, end_flag = 0;
  for (start = 0;; start++) {
    ds = fat_get_slot(dir, start, &bh, 1);
    if (end_flag || (ds->order == DIR_DELETE) || (ds->order == 0) ||
        ((u64)start * FAT_ENTRY_SIZE >= dir->i_size)) {
      // 如果为0应该已经不用找了(后面均为空闲)，但是要进行可能的FAT空间分配，所以不立即跳出
      if (end_flag == 0 &&
          ((ds->order == 0) || ((u64)start * FAT_ENTRY_SIZE >= dir->i_size)))
        end_flag = 1;
      if (count == 0) res = start;
      count++;
      if (count >= num) break;
    } else if (count > 0) {
      count = 0;
    }
  }
  if (bh) {
    brelse(bh);
  }
  return res;
}

PRIVATE int fat_check_short(struct inode* dir, const char* name) {
  buf_head* bh = NULL;
  struct fat_dir_slot* ds;
  int start, res = 0;
  for (start = 0; (u64)start * FAT_ENTRY_SIZE < dir->i_size; start++) {
    ds = fat_get_slot(dir, start, &bh, 0);
    if (ds->order == DIR_DELETE) {
      continue;
    } else if (ds->order == 0) {
      break;  // 后面的均为空闲，不用找了
    }
    if (!strncmp(name, ((struct fat_dir_entry*)ds)->name, 11)) {
      res = -1;
      break;
    }
  }
  if (bh) {
    brelse(bh);
  }
  return res;
}

PRIVATE int fat_check_empty(struct inode* dir) {
  buf_head* bh = NULL;
  struct fat_dir_slot* ds;
  int start, res = 0;
  for (start = 0; (u64)start * FAT_ENTRY_SIZE < dir->i_size; start++) {
    ds = fat_get_slot(dir, start, &bh, 0);
    if (ds->order == DIR_DELETE) {
      continue;
    } else if (ds->order == 0) {
      break;  // 后面的均为空闲，不用找了
    }
    if ((!strncmp(FAT_DOT, ((struct fat_dir_entry*)ds)->name, 11)) ||
        (!strncmp(FAT_DOTDOT, ((struct fat_dir_entry*)ds)->name, 11))) {
      continue;
    }
    res = -1;
    break;
  }
  if (bh) {
    brelse(bh);
  }
  return res;
}

// get shortname return 0 if legal
PRIVATE int fat_gen_shortname(struct inode* dir, const char* fullname,
                              char* shortname) {
  int len = strlen(fullname);
  int type = 0, offset = 0, ext_start = -1, baselen;
  char c;
  for (ext_start = len; ext_start >= 0; ext_start--) {
    if (fullname[ext_start] == '.') {
      ext_start++;
      break;
    }
  }
  // ext_start: 最后一个.
  // type: 0忽略的前缀字符;1 文件名(8); 2 扩展名(3)
  for (int i = 0; i < len; i++) {
    c = fullname[i];
    if (type == 0) {  // skipped start
      if (!(c == '.' || c == ' ')) {
        type = 1;
      }
      if (i == ext_start) {
        ext_start = -1;
      }
    } else if (type == 1 && ext_start != -1 && i >= ext_start) {
      type = 2;
      offset = 8;
    }
    // 跳过超出短文件名8.3的部分
    if (!((type == 1 && offset < 8 &&
           ((ext_start == -1) || (i < ext_start - 1))) ||
          (type == 2 && offset < 11)))
      continue;
    if (fat_skip_char(c)) {
      continue;
    }

    shortname[offset++] = c;
    if (type) {
      baselen = offset;
    }
  }
  if ((u8)shortname[0] == DIR_DELETE) {
    shortname[0] = 0x05;
  }
  if (!fat_check_short(dir, shortname)) return 0;
  if (baselen > 6) {
    baselen = 6;
  }
  shortname[baselen] = '~';
  for (int i = 0; i < 10; i++) {
    shortname[baselen + 1] = '0' + i;
    if (!fat_check_short(dir, shortname)) {
      return 0;
    }
  }
  // 尝试达到10次仍然重复,使用tick产生随机序列
  // 事实上，现有已知的实现大多也是这么处理的
  int rand = kern_get_ticks();
  itoa(rand & 0xFFFF, shortname + baselen - 4, 16);
  shortname[baselen] = '~';
  shortname[baselen + 1] = '0' + rand % 10;
  if (!fat_check_short(dir, shortname)) {
    return 0;
  }
  return -1;
}

PRIVATE int fat_write_shortname(struct inode* dir, struct fat_dir_entry* entry,
                                int order) {
  buf_head* bh = NULL;
  struct fat_dir_entry* de =
      (struct fat_dir_entry*)fat_get_slot(dir, order, &bh, 1);
  if (!de) return -1;
  *de = *entry;
  mark_buff_dirty(bh);
  brelse(bh);
  return 0;
}

PRIVATE struct inode* fat_add_entry(struct inode* dir, const char* name,
                                    int is_dir, u32 timestamp) {
  int nslot = (strlen(name) + 12) / 13;
  int free_slot_order = fat_find_free(dir, nslot + 1);
  buf_head* bh = NULL;
  u8 chksum;
  int offset;
  char shortname[12];
  u16 uniname[256];
  memset(shortname, ' ', 12);
  shortname[11] = 0;
  fat_gen_shortname(dir, name, shortname);
  memset(uniname, -1, 256);
  char2uni(uniname, name, 256);
  chksum = fat_chksum(shortname);
  for (int slot = nslot; slot > 0; slot--) {  // 写入长目录项
    u8 order = slot | ((slot == nslot) ? DIR_LDIR_END : 0);
    struct fat_dir_slot* ds =
        fat_get_slot(dir, free_slot_order + (nslot - slot), &bh, 1);
    ds->attr = ATTR_LNAME;
    ds->order = order;
    offset = (slot - 1) * 13;
    memcpy(ds->name0_4, uniname + offset, 10);
    offset += 5;
    memcpy(ds->name5_10, uniname + offset, 12);
    offset += 6;
    memcpy(ds->name11_12, uniname + offset, 4);
    ds->checksum = chksum;
    mark_buff_dirty(bh);
  }
  if (bh) brelse(bh);
  dir->i_mtime = timestamp;
  struct inode* inode = vfs_new_inode(dir->i_sb);
  inode->i_dev = dir->i_sb->sb_dev;
  inode->i_no = fat_ino(dir, free_slot_order + nslot);
  inode->i_atime = inode->i_crtime = inode->i_mtime = timestamp;
  fill_fat_info(dir->i_sb, 0, &FAT_INODE(inode)->fat_info);
  struct fat_dir_entry de;
  memset(&de, 0, sizeof(struct fat_dir_entry));
  memcpy(de.name, shortname, 11);
  de.attr = (is_dir) ? ATTR_DIR : ATTR_ARCH;
  fat_update_datetime(timestamp, &de.cdate, &de.ctime, &de.ctime_10ms);
  // fat_update_datetime(timestamp, &de.mdate, &de.mtime);
  // fat_update_datetime(timestamp, &de.adate, NULL);
  fat_write_shortname(dir, &de, free_slot_order + nslot);
  if ((u64)(free_slot_order + nslot + 1) * FAT_ENTRY_SIZE > dir->i_size) {
    dir->i_size = (free_slot_order + nslot + 1) * FAT_ENTRY_SIZE;
    memset(&de, 0, sizeof(struct fat_dir_entry));
    fat_write_shortname(dir, &de, free_slot_order + nslot + 1);
  }
  fat32_sync_inode(dir);
  return inode;
}

PUBLIC void fat32_read_inode(struct inode* inode) {
  struct super_block* sb = inode->i_sb;
  buf_head* bh = NULL;
  struct fat_dir_entry* entry = NULL;
  int cluster_start;
  int ino = inode->i_no;
  if (ino == FAT_ROOT_INO) {
    cluster_start = FAT_SB(sb)->root_cluster;
    inode->i_mode = I_RWX;
    inode->i_type = I_DIRECTORY;
    inode->i_crtime = 0;
    inode->i_mtime = 0;
    inode->i_atime = 0;
  } else {
    int entry_block = sb->sb_blocksize / FAT_ENTRY_SIZE;
    entry = &((struct fat_dir_entry*)fat_bread(sb, ino / entry_block,
                                               &bh))[ino % entry_block];
    cluster_start = entry->start_l | (entry->start_h << 16);
    inode->i_mode = (entry->attr & ATTR_RO) ? I_R | I_X : I_RWX;
    inode->i_type = (entry->attr & ATTR_DIR) ? I_DIRECTORY : I_REGULAR;
    inode->i_size = entry->size;
    inode->i_crtime =
        fat_read_datetime(entry->cdate, entry->ctime, entry->ctime_10ms);
    inode->i_mtime = fat_read_datetime(entry->mdate, entry->mtime, 0);
    inode->i_atime = fat_read_datetime(entry->adate, 0, 0);
  }
  inode->i_sb = sb;
  inode->i_dev = sb->sb_dev;
  inode->i_no = ino;
  int cnt = fill_fat_info(sb, cluster_start, &FAT_INODE(inode)->fat_info);
  if (!inode->i_size) {
    inode->i_size = cnt * FAT_SB(sb)->cluster_block * sb->sb_blocksize;
  }
  inode->i_nlink = 1;
  inode->i_op = &fat32_inode_ops;
  inode->i_fop = &fat32_file_ops;
  if (bh) brelse(bh);
}

PUBLIC int fat32_sync_inode(struct inode* inode) {
  if (inode->i_no == FAT_ROOT_INO) {
    return 0;
  }
  struct super_block* sb = inode->i_sb;
  int entry_block = sb->sb_blocksize / FAT_ENTRY_SIZE;
  buf_head* bh = NULL;
  u32 ino = inode->i_no;
  int start = FAT_INODE(inode)->fat_info->cluster_start;
  struct fat_dir_entry* de = &((struct fat_dir_entry*)fat_bread(
      inode->i_sb, ino / entry_block, &bh))[ino % entry_block];
  if (!de) {
    return -1;
  }
  de->size = inode->i_size;
  if (!((de->start_h << 16) | de->start_l) && start) {
    de->start_h = start >> 16;
    de->start_l = start & 0xFFFF;
  }
  fat_update_datetime(inode->i_atime, &de->adate, NULL, NULL);
  fat_update_datetime(inode->i_mtime, &de->mdate, &de->mtime, NULL);
  if (bh) {
    mark_buff_dirty(bh);
    brelse(bh);
  }
  // sync_buffers(0);
  return 0;
}

PUBLIC void fat32_put_inode(struct inode* inode) {
  struct fat_info* next;
  for (struct fat_info* ent = FAT_INODE(inode)->fat_info; ent; ent = next) {
    next = ent->next;
    free_fat_info(ent);
  }
}

PUBLIC void fat32_delete_inode(struct inode* inode) {
  struct fat_info* ent = FAT_INODE(inode)->fat_info;
  if (!ent->cluster_start) {  // inode 并没有分配磁盘空间
    return;
  }
  while (ent) {
    for (int clus_in_ent = 0; clus_in_ent < ent->length; clus_in_ent++) {
      read_write_fat(inode->i_sb, ent->cluster_start + clus_in_ent, 0);
    }
    ent = ent->next;
  }
}

PUBLIC struct dentry* fat32_lookup(struct inode* dir, const char* filename) {
  // 1. format
  // 2. loop get entry until match
  char full_name[256];  // for long dir store real name
  int entry = 0;
  u32 ino = 0;
  struct fat_dir_entry* de = NULL;
  buf_head* bh = NULL;
  struct dentry* dentry = NULL;
  for (; (u64)entry * FAT_ENTRY_SIZE < dir->i_size; entry++) {
    de = fat_get_entry(dir, &entry, &bh, full_name);
    if (!de) break;
    if (!stricmp(filename, full_name)) {
      ino = fat_ino(dir, entry);
      break;
    }
  }
  if (bh) {
    brelse(bh);
  }
  if (ino) {
    struct inode* inode = vfs_get_inode(dir->i_sb, ino);
    dentry = vfs_new_dentry(full_name, inode);
    dentry->d_op = &fat32_dentry_ops;
  }
  return dentry;
}

PUBLIC int fat32_create(struct inode* dir, struct dentry* dentry, int mode) {
  // struct tm time;
  u32 timestamp = current_timestamp;
  char* name = dentry_name(dentry);
  if (fat_badname(name)) {
    return -1;
  }
  struct inode* inode = fat_add_entry(dir, name, 0, timestamp);
  inode->i_type = I_REGULAR;
  inode->i_mode = mode;
  inode->i_op = &fat32_inode_ops;
  inode->i_fop = &fat32_file_ops;
  dentry->d_inode = inode;
  fat32_sync_inode(inode);
  return 0;
}

PRIVATE int fat32_unlink_name(struct inode* dir, struct dentry* dentry) {
  buf_head* bh = NULL;
  struct fat_dir_slot* ds;
  struct inode* inode = dentry->d_inode;
  u32 ino = inode->i_no;
  u8 chksum;
  if (ino == FAT_ROOT_INO) {
    return -1;
  }
  int pos = fat_entry_offset_by_ino(dir, ino);
  if (pos == -1) {
    return -1;
  }
  ds = fat_get_slot(dir, pos, &bh, 0);
  chksum = fat_chksum(((struct fat_dir_entry*)ds)->name);
  do {
    ds->order = DIR_DELETE;
    mark_buff_dirty(bh);
    pos--;
    ds = fat_get_slot(dir, pos, &bh, 0);
  } while (ds->checksum == chksum);
  if (bh) {
    brelse(bh);
  }
  return 0;
}

PUBLIC int fat32_unlink(struct inode* dir, struct dentry* dentry) {
  return fat32_unlink_name(dir, dentry);
}

PUBLIC int fat32_rmdir(struct inode* dir, struct dentry* dentry) {
  if (fat_check_empty(dentry->d_inode)) {
    return -1;
  }
  return fat32_unlink_name(dir, dentry);
}

PUBLIC int fat32_mkdir(struct inode* dir, struct dentry* dentry, int mode) {
  // struct tm time;
  struct super_block* sb = dir->i_sb;
  u32 timestamp = current_timestamp;
  char* name = dentry_name(dentry);
  if (fat_badname(name)) {
    return -1;
  }
  struct inode* inode = fat_add_entry(dir, name, 1, timestamp);
  inode->i_type = I_DIRECTORY;
  inode->i_mode = mode;
  inode->i_op = &fat32_inode_ops;
  inode->i_fop = &fat32_file_ops;
  inode->i_size = 2 * FAT_ENTRY_SIZE;
  struct fat_dir_entry de;
  memset(&de, 0, sizeof(struct fat_dir_entry));
  memcpy(de.name, FAT_DOT, 11);
  de.attr = ATTR_DIR;
  de.size = FAT_SB(sb)->cluster_block * sb->sb_blocksize;
  fat_update_datetime(timestamp, &de.cdate, &de.ctime, &de.ctime_10ms);
  fat_update_datetime(timestamp, &de.mdate, &de.mtime, NULL);
  fat_update_datetime(timestamp, &de.adate, NULL, NULL);
  fat_write_shortname(inode, &de, 0);
  memcpy(de.name, FAT_DOTDOT, 11);
  de.attr = ATTR_DIR;
  de.size = dir->i_size;
  fat_update_datetime(dir->i_crtime, &de.cdate, &de.ctime, &de.ctime_10ms);
  fat_update_datetime(dir->i_mtime, &de.mdate, &de.mtime, NULL);
  fat_update_datetime(dir->i_atime, &de.adate, NULL, NULL);
  fat_write_shortname(inode, &de, 1);
  memset(&de, 0, sizeof(struct fat_dir_entry));
  fat_write_shortname(inode, &de, 2);
  dentry->d_inode = inode;
  dentry->d_op = &fat32_dentry_ops;
  fat32_sync_inode(inode);
  return 0;
}

PUBLIC int fat32_read(struct file_desc* file, unsigned int count, char* buf) {
  struct inode* inode = file->fd_dentry->d_inode;
  u64 start, pos, end, len;
  buf_head* bh = NULL;
  char* file_buf;
  start = file->fd_pos;
  end = min(start + count, inode->i_size);
  for (pos = start; pos < end;) {
    len = min(SECTOR_SIZE - (pos & (SECTOR_SIZE - 1)), end - pos);
    file_buf = fat_get_data(inode, pos, &bh, 0);
    if (!file_buf) {
      break;
    }
    memcpy(buf + pos - start, file_buf, len);
    pos += len;
  }
  if (bh) {
    brelse(bh);
  }
  inode->i_atime = current_timestamp;
  file->fd_pos = pos;
  return pos - start;
}

PUBLIC int fat32_write(struct file_desc* file, unsigned int count,
                       const char* buf) {
  struct inode* inode = file->fd_dentry->d_inode;
  u64 start, pos, end, len;
  buf_head* bh = NULL;
  char* file_buf;
  start = file->fd_pos;
  end = start + count;
  for (pos = start; pos < end;) {
    len = min(SECTOR_SIZE - (pos & (SECTOR_SIZE - 1)), end - pos);
    file_buf = fat_get_data(inode, pos, &bh, 1);
    if (!file_buf) {
      break;
    }
    memcpy(file_buf, buf + pos - start, len);
    mark_buff_dirty(bh);
    pos += len;
  }
  if (bh) {
    brelse(bh);
  }
  if (pos > inode->i_size) {
    inode->i_size = pos;
  }
  inode->i_mtime = current_timestamp;
  fat32_sync_inode(inode);
  file->fd_pos = pos;
  return pos - start;
}

PUBLIC int fat32_readdir(struct file_desc* file, unsigned int count,
                         struct dirent* start) {
  char full_name[256];  // for long dir store real name
  int entry = 0;
  struct fat_dir_entry* de;
  buf_head* bh = NULL;
  struct inode* dir = file->fd_dentry->d_inode;
  struct dirent* dent = start;
  if (dir->i_no == FAT_ROOT_INO) {
    dirent_fill(dent, FAT_ROOT_INO, 1);
    strcpy(dent->d_name, ".");
    count -= dent->d_len;
    dent = dirent_next(dent);
    dirent_fill(dent, FAT_ROOT_INO, 2);
    strcpy(dent->d_name, "..");
    count -= dent->d_len;
    dent = dirent_next(dent);
  }
  for (; (u64)entry * FAT_ENTRY_SIZE < dir->i_size; entry++) {
    de = fat_get_entry(dir, &entry, &bh, full_name);
    if (!de) break;
    if (count < dirent_len(strlen(full_name))) {
      break;
    }
    dirent_fill(dent, fat_ino(dir, entry), strlen(full_name));
    strcpy(dent->d_name, full_name);
    count -= dent->d_len;
    dent = dirent_next(dent);
  }
  if (bh) {
    brelse(bh);
  }
  return (u32)dent - (u32)start;
}

PUBLIC int fat32_fill_superblock(struct super_block* sb, int dev) {
  int dir_blocks, fsinfo_blk;
  buf_head* fs_info_buf;
  buf_head* bh = bread(dev, 0);
  struct fat32_bpb* bpb = (struct fat32_bpb*)bh->buffer;
  struct fat32_sb_info* sbi = FAT_SB(sb);
  sb->sb_blocksize = bpb->BytsPerSec;
  if (bpb->FATSz16 != 0) {
    // not FAT32 volume, current not supported
    brelse(bh);
    return -1;
  } else {
    sbi->fat_bit = 32;
    sbi->fat_size = bpb->FATSz32;
  }
  if (bpb->TotSec16 != 0) {
    sbi->tot_block = bpb->TotSec16;
  } else {
    sbi->tot_block = bpb->TotSec32;
  }
  sbi->fat_num = bpb->NumFATs;
  sbi->cluster_block = bpb->SecPerClus;
  sbi->fat_start_block = bpb->RsvdSecCnt;
  sbi->dir_start_block = sbi->fat_start_block + bpb->NumFATs * sbi->fat_size;
  dir_blocks =
      (((bpb->RootEntCnt * 32) + bpb->BytsPerSec - 1) / bpb->BytsPerSec);
  // for FAT32, dir_blocks should be 0
  sbi->data_start_block = sbi->dir_start_block + dir_blocks;
  sbi->max_cluster =
      (sbi->tot_block - sbi->data_start_block) / sbi->cluster_block + 1;
  sbi->root_cluster = bpb->RootClus;
  sbi->fsinfo_block = bpb->FSInfo;
  fsinfo_blk = sbi->fsinfo_block / (BLOCK_SIZE / sb->sb_blocksize);
  if (fsinfo_blk) {
    fs_info_buf = bread(dev, fsinfo_blk);
  } else {
    fs_info_buf = bh;
  }
  sbi->fsinfo = *((struct fat_fsinfo*)((char*)fs_info_buf->buffer +
                                       (sb->sb_blocksize * sbi->fsinfo_block) +
                                       FS_INFO_SECTOR_OFFSET));
  if (sbi->fsinfo.cluster_next_free == -1) {  // may invalid(-1)
    sbi->fsinfo.cluster_next_free = 2;
  }
  if (fsinfo_blk) {
    brelse(fs_info_buf);
  }
  brelse(bh);
  initlock(&sb->lock, "FAT");
  sb->sb_dev = dev;
  sb->fs_type = FAT32_TYPE;
  sb->sb_op = &fat32_sb_ops;
  struct inode* fat32_root = vfs_get_inode(sb, FAT_ROOT_INO);
  sb->sb_root = vfs_new_dentry("/", fat32_root);
  sb->sb_root->d_op = &fat32_dentry_ops;
  return 0;
}

void fat32_query_size_info(struct fs_size_info* info) {
  info->sb_size = sizeof(struct fat32_sb_info);
  info->inode_size = sizeof(struct fat32_inode_info);
}

struct superblock_operations fat32_sb_ops = {
    .query_size_info = fat32_query_size_info,
    .fill_superblock = fat32_fill_superblock,
    .read_inode = fat32_read_inode,
    .sync_inode = fat32_sync_inode,
    .put_inode = fat32_put_inode,
    .delete_inode = fat32_delete_inode,
};

struct inode_operations fat32_inode_ops = {
    .lookup = fat32_lookup,
    .create = fat32_create,
    .unlink = fat32_unlink,
    .mkdir = fat32_mkdir,
    .rmdir = fat32_rmdir,
    .get_block = fat32_get_block,
};

struct dentry_operations fat32_dentry_ops = {
    .compare = stricmp,
};

struct file_operations fat32_file_ops = {
#ifdef OPT_PAGE_CACHE
    .read = generic_file_read,
    .write = generic_file_write,
    .fsync = generic_file_fsync,
#else
    .read = fat32_read,
    .write = fat32_write,
    .fsync = NULL,
#endif
    .readdir = fat32_readdir,
};
// unit test

#ifdef FAT_TEST
#include <stdio.h>
int fat_gen_shortname(const char* fullname, char* shortname) {
  int len = strlen(fullname);
  int type = 0, offset = 0, ext_start = -1;
  char c;
  for (ext_start = len; ext_start >= 0; ext_start--) {
    if (fullname[ext_start] == '.') {
      ext_start++;
      break;
    }
  }
  for (int i = 0; i < len; i++) {
    c = fullname[i];
    if (type == 0) {  // skipped start
      if (!(c == '.' || c == ' ')) {
        type = 1;
      }
      if (i == ext_start) {
        ext_start = -1;
      }
    } else if (type == 1 && ext_start != -1 && i >= ext_start) {
      type = 2;
      offset = 8;
    }
    if (!((type == 1 && offset < 8 &&
           ((ext_start == -1) || (i < ext_start - 1))) ||
          (type == 2 && offset < 11)))
      continue;
    if (c >= 'a' && c <= 'z') {
      c = c - 'a' + 'A';
    }
    shortname[offset++] = c;
  }
  shortname[11] = 0;
}

int main(int argc, char* argv[]) {
  char buf[256];
  char out[256];
  while (1) {
    memset(out, ' ', 256);
    gets(buf);
    fat_gen_shortname(buf, out);
    puts(out);
  }
}
#endif
