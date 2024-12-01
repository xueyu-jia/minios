#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "orangefs.h"
#include "orangefs_disk.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: %s <device>", argv[0]);
    return 1;
  }

  if (disk_open(argv[1]) != 0) {
    fprintf(stderr, "failed to open device file");
    return -1;
  }

  int i;
  int total_blks = disk_size() / ORANGE_BLK_SIZE;
  char fsbuf[ORANGE_BLK_SIZE];
  struct orange_sb sb;
  sb.magic = ORANGE_MAGIC;
  sb.nr_inodes = 4096;
  sb.nr_inode_blocks = sb.nr_inodes * ORANGE_INODE_SIZE / ORANGE_BLK_SIZE;
  sb.nr_blocks = total_blks; /* partition size in block */
  sb.nr_imap_blocks = 1;
  sb.nr_smap_blocks =
      (sb.nr_blocks - 2 - sb.nr_inode_blocks) / (ORANGE_BLK_SIZE * 8) + 1;
  sb.n_1st_block = 1 + 1 + /* boot sector & super block */
                   sb.nr_imap_blocks + sb.nr_smap_blocks + sb.nr_inode_blocks;
  sb.root_inode = 1;
  sb.inode_size = ORANGE_INODE_SIZE;

  sb.inode_isize_off = offsetof(struct orange_inode, i_size);
  sb.inode_start_off = offsetof(struct orange_inode, i_start_block);
  sb.dir_ent_size = ORANGE_DIRENT_SIZE;

  sb.dir_ent_inode_off = offsetof(struct orange_direntry, inode_nr);
  sb.dir_ent_fname_off = offsetof(struct orange_direntry, name);

  memset(fsbuf, 0x90, ORANGE_BLK_SIZE);
  memcpy(fsbuf, &sb, sizeof(sb));
  disk_write(1, 0, ORANGE_BLK_SIZE, fsbuf);
  orangefs_fillsuper();

  /*      inode map       */
  memset(fsbuf, 0, ORANGE_BLK_SIZE);
  fsbuf[0] = 1;  // inode 0 reserved as invalid
  disk_write(2, 0, ORANGE_BLK_SIZE, fsbuf);

  /*      secter map      */
  memset(fsbuf, 0, ORANGE_BLK_SIZE);
  for (i = 0; i < sb.nr_smap_blocks; i++) {
    if ((i + 1) * ORANGE_BLK_SIZE * 8 > total_blks) {
      int off = max(total_blks - (i)*ORANGE_BLK_SIZE * 8, 0) / 8;
      memset(fsbuf + off, 0xFF, ORANGE_BLK_SIZE - off);
    }
    disk_write(2 + sb.nr_imap_blocks + i, 0, ORANGE_BLK_SIZE, fsbuf);
  }

  memset(fsbuf, 0, ORANGE_BLK_SIZE);
  for (i = 0; i < sb.nr_inode_blocks; i++) {
    disk_write(2 + sb.nr_imap_blocks + sb.nr_smap_blocks + i, 0,
               ORANGE_BLK_SIZE, fsbuf);
  }

  // root
  int root = orangefs_allocinode();
  struct orange_inode *pin = orangefs_iget(root);
  pin->i_size = 0;
  pin->i_mode = I_DIRECTORY;
  orangefs_allocsector(pin);
  orangefs_syncinode(root, pin);
  orangefs_add_direntry(root, ".", root);
  orangefs_free();
  return 0;
}