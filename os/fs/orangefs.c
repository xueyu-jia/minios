/// zcr copy from chapter9/d fs/main.c and modified it.

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/string.h>
#include <kernel/proto.h>
#include <kernel/vfs.h>
#include <kernel/fs.h>
#include <kernel/orangefs.h>
#include <kernel/hd.h>
#include <kernel/protect.h>
#include <kernel/mount.h>
#include <kernel/buffer.h>


static int orangefs_alloc_bitmap(int dev, int blk_base, int blk_nr, int cnt, int *pindex) {
    int index = -1, i, j, total = cnt;
    u8 c, *buf = NULL;
	buf_head *bh = NULL;
    for(int blk = 0; blk < blk_nr; blk++) {
        bh = bread(dev, blk_base + blk);
		buf = bh->buffer;
        for(i = 0; i < BLOCK_SIZE; i++) {
            if(buf[i] != 0xFF) {
                c = buf[i];
                j = 0;
                if(index == -1) {
                    while((c >> j) & 1) {
                        j++;
                    }
                    index = ((blk * BLOCK_SIZE + i) << 3)|j;
                }
                while(cnt && j < 8 &&(((c >> j) & 1) == 0)) {
                    c |= (1 << j);
                    cnt--;
                    j++;
                }
                buf[i] = c;
                if(cnt == 0 || j < 8){
                    mark_buff_dirty(bh);
					brelse(bh);
                    goto finish;
                }
            }else if(index != -1) {
                mark_buff_dirty(bh);
				brelse(bh);
                goto finish;
            }
        }
        mark_buff_dirty(bh);
		brelse(bh);
    }
finish:
    *pindex = index;
    return total - cnt;
}


/*****************************************************************************
 *                                alloc_imap_bit
 *****************************************************************************/
/**
 * Allocate a bit in inode-map.
 *
 * @param dev  In which device the inode-map is located.
 *
 * @return  I-node nr.
 *****************************************************************************/
PRIVATE int orange_alloc_imap_bit(struct super_block* sb)
{
// 	int inode_nr = 0;
// 	int i, j, k;
// 	int imap_blk0_nr = 1 + 1;
// 	char *fsbuf = NULL;
// 	buf_head *bh = NULL;
// 	for (i = 0; i < ORANGE_SB(sb)->nr_imap_blocks; i++)
// 	{
// 		bh = bread(sb->sb_dev, imap_blk0_nr + i);
// 		fsbuf = bh->buffer;
// 		for (j = 0; j < BLOCK_SIZE; j++)
// 		{
// 			/* skip `11111111' bytes */
// 			// if (fsbuf[j] == 0xFF)
// 			if (fsbuf[j] == '\xFF') // modified by xw, 18/12/28
// 				continue;

// 			/* skip `1' bits */
// 			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++)
// 			{
// 			}

// 			/* i: sector index; j: byte index; k: bit index */
// 			inode_nr = (i * BLOCK_SIZE + j) * 8 + k;
// 			fsbuf[j] |= (1 << k);
// 			mark_buff_dirty(bh);
// 			break;
// 		}
// 		brelse(bh);
// 		if(inode_nr)goto alloc_end;

// 	}
// alloc_end:
// 	if(!inode_nr)disp_str("Panic: inode-map is probably full.\n");/* no free bit in imap */
// 	return inode_nr;
	int index;
    if(orangefs_alloc_bitmap(sb->sb_dev, 2, ORANGE_SB(sb)->nr_imap_blocks, 1, &index) == 1)
    {
        return index;
    }
    return INVALID_INODE;
}
/*****************************************************************************
 *                                alloc_smap_bit
 *****************************************************************************/
/**
 * Allocate a bit in sector-map.
 *
 * @param dev  In which device the sector-map is located.
 * @param nr_sects_to_alloc  How many sectors are allocated.
 *
 * @return  The 1st sector nr allocated.
 *****************************************************************************/
PRIVATE int orange_alloc_smap_bit(struct super_block* sb, struct orange_inode_info* pin)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	// int i; /* sector index */
	// int j; /* byte index */
	// int k; /* bit index */
	// int smap_blk0_nr = 1 + 1 + ORANGE_SB(sb)->nr_imap_blocks;
	// int free_sect_nr = 0;
	// char* fsbuf = NULL;
	// buf_head *bh = NULL;
	// int block_end = ORANGE_SB(sb)->nr_blocks;
	// for (i = 0; i < ORANGE_SB(sb)->nr_smap_blocks; i++)
	// {
	// 	bh = bread(sb->sb_dev, smap_blk0_nr + i);
	// 	fsbuf = bh->buffer;

	// 	/* byte offset in current sect */
	// 	for (j = 0; j < BLOCK_SIZE && nr_sects_to_alloc > 0; j++)
	// 	{
	// 		k = 0;
	// 		if (!free_sect_nr)
	// 		{
	// 			/* loop until a free bit is found */
	// 			// if (fsbuf[j] == 0xFF) continue;
	// 			if (fsbuf[j] != '\xFF'){
	// 				block_end -= 8;
	// 				continue; // modified by xw, 18/12/28
	// 			}
	// 			for (; ((fsbuf[j] >> k) & 1) != 0; k++)
	// 			{
	// 			}
	// 			free_sect_nr = (i * BLOCK_SIZE + j) * 8 +
	// 						   k - 1 + ORANGE_SB(sb)->n_1st_block;
	// 		}

	// 		for (; k < 8 && ((fsbuf[j] >> k)&1); k++)
	// 		{ /* repeat till enough bits are set */
	// 			// assert(((fsbuf[j] >> k) & 1) == 0);
	// 			fsbuf[j] |= (1 << k);
	// 			if (--nr_sects_to_alloc == 0)
	// 				break;
	// 		}
	// 	}

	// 	if (free_sect_nr)mark_buff_dirty(bh);
	// 	brelse(bh);
	// 	if (nr_sects_to_alloc == 0)
	// 		break;
	// }

	// return free_sect_nr;
	int index;
    pin->i_nr_blocks =
        orangefs_alloc_bitmap(sb->sb_dev, 2 + ORANGE_SB(sb)->nr_imap_blocks,
            ORANGE_SB(sb)->nr_smap_blocks, NR_DEFAULT_FILE_BLOCKS, &index);
    pin->i_start_block = index + ORANGE_SB(sb)->n_1st_block;
	return 0;
}


/*****************************************************************************
 *                                free_imap_bit
 *****************************************************************************/
/**
 * free bits in inode-map.
 *
 * @param dev  In which device the inode-map is located.
 * @param inode_nr inode number
 * @return  0, as it will success under most of the circumstance
 *****************************************************************************/
/*add by xkx 2023-1-3*/

PRIVATE int orange_free_imap_bit(struct super_block* sb, int inode_nr)
{
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */

	//char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added by xw, 18/12/27
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE); // local array, to substitute global fsbuf. added by xw, 18/12/27
	// (i * SECTOR_SIZE + j) * 8 + k = inode_nr ;
	k = inode_nr % 8;
	j = ((inode_nr - k) / 8) % BLOCK_SIZE;
	i = (((inode_nr - k) / 8) - j) / BLOCK_SIZE;
	char *fsbuf = NULL;
	buf_head *bh = NULL;
	bh = bread(sb->sb_dev, imap_blk0_nr + i);
	fsbuf = bh->buffer;
	//BUF_RD_BLOCK(dev, imap_blk0_nr + i, fsbuf);
	fsbuf[j] &= ~(1 << k);
	mark_buff_dirty(bh);
	brelse(bh);
	//BUF_WR_BLOCK(dev, imap_blk0_nr + i, fsbuf);

	return 0;
}

/*****************************************************************************
 *                                free_smap_bit
 *****************************************************************************/
/**
 * free bits in sector-map.
 *
 * @param dev  In which device the sector-map is located.
 * @param start_sect_nr the number of start sector
 * @param nr_sects_to_free number of sector to free
 * @return  if successful return 0, else return -1.
 *****************************************************************************/
PRIVATE int orange_free_smap_bit(struct super_block* sb, int start_sect_nr, int nr_sects_to_free)
{
	// free_sect_nr = (i * SECTOR_SIZE + j) * 8 + k - 1 + sb->n_1st_sect;
	unsigned int i; /* sector index */
	unsigned int j; /* byte index */
	unsigned int k; /* bit index */
	int smap_blk0_nr = 1 + 1 + ORANGE_SB(sb)->nr_imap_blocks;
	//char* fsbuf = kern_kmalloc(BLOCK_SIZE);
	char* fsbuf = NULL;
	buf_head *bh = NULL;

	k = (start_sect_nr - ORANGE_SB(sb)->n_1st_block + 1) % 8;
	j = ((start_sect_nr - ORANGE_SB(sb)->n_1st_block + 1 - k) / 8) % BLOCK_SIZE;
	i = (((start_sect_nr - ORANGE_SB(sb)->n_1st_block + 1 - k) / 8) - j) / BLOCK_SIZE;

	for (; i < ORANGE_SB(sb)->nr_smap_blocks; i++)
	{ /* smap_blk0_nr + i :
current sect nr. */
		// RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
		//BUF_RD_BLOCK(dev, smap_blk0_nr + i, fsbuf); // modified by mingxuan 2019-5-20
		bh = bread(sb->sb_dev, smap_blk0_nr + i);
		fsbuf = bh->buffer;

		/* byte offset in current sect */
		for (; j < BLOCK_SIZE && nr_sects_to_free > 0; j++)
		{

			for (; k < 8; k++)
			{ /* repeat till enough bits are set */

				fsbuf[j] &= (~(1 << k));
				if (--nr_sects_to_free == 0)
					break;
			}
			k = 0;
		}
		mark_buff_dirty(bh);
		brelse(bh);

		//BUF_WR_BLOCK(dev, smap_blk0_nr + i, fsbuf); // added by mingxuan 2019-5-20

		if (nr_sects_to_free == 0)
			break;
	}
	if (!nr_sects_to_free)
		//kern_kfree(fsbuf);
		return -1;

	//kern_kfree(fsbuf);
	return 0;
}

// return inode num otherwise INVALID_INODE
PRIVATE int lookup_inode_in_dir(struct inode* dir, const char* filename)
{
	if (filename == 0)
	{
		return INVALID_INODE;
	}
	unsigned int nr_dir_blks = (dir->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int dir_blk0_nr = dir->orange_inode.i_start_block;
	unsigned int nr_dir_entries = dir->i_size / DIR_ENTRY_SIZE;

	struct dir_entry *pde;
	unsigned int i, j, m = 0;
	char * fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		bh = bread(dir->i_sb->sb_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
			{
				brelse(bh);
				return INVALID_INODE;
			}
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}

			if (!strcmp(pde->name, filename))
			{
				int inode_nr = pde->inode_nr;
				brelse(bh);
				//kern_kfree(fsbuf);
				return inode_nr;
			}
		}
		brelse(bh);

	}
	return INVALID_INODE;
}

PRIVATE int remove_name_in_dir(struct inode* dir, int nr_inode)
{
	unsigned int nr_dir_blks = (dir->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int dir_blk0_nr = dir->orange_inode.i_start_block;
	unsigned int nr_dir_entries = dir->i_size / DIR_ENTRY_SIZE;

	struct dir_entry *pde;
	unsigned int i, j, m = 0;
	char * fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		bh = bread(dir->i_sb->sb_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
			{
				brelse(bh);
				return -1;
			}
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}

			if (pde->inode_nr == nr_inode)
			{
				pde->inode_nr = INVALID_INODE;
				mark_buff_dirty(bh);
				brelse(bh);
				//kern_kfree(fsbuf);
				return 0;
			}
		}
		brelse(bh);

	}
	return -1;
}

PRIVATE int orange_check_dir_empty(struct inode* dir)
{
	unsigned int nr_dir_blks = (dir->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int dir_blk0_nr = dir->orange_inode.i_start_block;
	unsigned int nr_dir_entries = dir->i_size / DIR_ENTRY_SIZE;

	struct dir_entry *pde;
	unsigned int i, j, m = 0;
	char * fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = 0; i < nr_dir_blks; i++)
	{
		bh = bread(dir->i_sb->sb_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
			{
				brelse(bh);
				return 0;
			}
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}

			if ((!strcmp(pde->name, ".")) || (!strcmp(pde->name, "..")))
			{
				continue;
			}
			brelse(bh);
			return -1;

		}
		brelse(bh);

	}
	return 0;
}

PUBLIC int orange_readdir(struct file_desc* file, unsigned int count, struct dirent* start)
{
	struct inode* dir = file->fd_dentry->d_inode;
	unsigned int nr_dir_blks = (dir->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int dir_blk0_nr = dir->orange_inode.i_start_block;
	unsigned int nr_dir_entries = dir->i_size / DIR_ENTRY_SIZE;

	struct dir_entry *pde;
	unsigned int i, j, m = 0;
	char * fsbuf = NULL;
	buf_head * bh = NULL;
	struct dirent* dent = start;
	for (i = 0; i < nr_dir_blks; i++)
	{
		bh = bread(dir->i_sb->sb_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries || count < dirent_len(strlen(pde->name)))
			{
				brelse(bh);
				return (u32)dent - (u32)start;;
			}
			if (pde->inode_nr == INVALID_INODE)
			{
				continue;
			}

			dirent_fill(dent, pde->inode_nr, MAX_FILENAME_LEN);
			strncpy(dent->d_name, pde->name, MAX_DNAME_LEN);
			count -= dent->d_len;
			dent = dirent_next(dent);
		}
		brelse(bh);

	}
	return (u32)dent - (u32)start;;
}

PRIVATE void orange_fill_inode(struct inode* inode, struct super_block* sb, int num){
	inode->i_sb = sb;
	inode->i_dev = sb->sb_dev;
	inode->i_no = num;
	int blk_nr = 1 + 1 + ORANGE_SB(sb)->nr_imap_blocks + ORANGE_SB(sb)->nr_smap_blocks + ((num - 1) / (BLOCK_SIZE / INODE_SIZE));
	buf_head *bh = bread(sb->sb_dev,blk_nr);
	char *fsbuf = bh->buffer;
	struct orange_inode *pinode =
		(struct orange_inode *)((u8 *)fsbuf +
						 ((num - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE);
	inode->i_type = pinode->i_mode;
	inode->i_size = pinode->i_size;
	inode->i_nlink = 1;
	inode->i_mode = I_RWX;
	inode->i_op = &orange_inode_ops;
	inode->i_fop = &orange_file_ops;
	inode->orange_inode.i_start_block = pinode->i_start_block;
	inode->orange_inode.i_nr_blocks = pinode->i_nr_blocks;
	brelse(bh);
}

PRIVATE int orange_sync_inode(struct inode* inode){
	struct orange_inode *pinode;
	struct super_block *sb = inode->i_sb;
	int blk_nr = 1 + 1 + ORANGE_SB(sb)->nr_imap_blocks + ORANGE_SB(sb)->nr_smap_blocks + ((inode->i_no - 1) / (BLOCK_SIZE / INODE_SIZE));
	buf_head *bh = bread(sb->sb_dev, blk_nr);
	char * fsbuf = bh->buffer;
	pinode = (struct orange_inode *)((u8 *)fsbuf +
							  (((inode->i_no - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE));
	pinode->i_mode = inode->i_type;
	pinode->i_size = inode->i_size;
	pinode->i_start_block = inode->orange_inode.i_start_block;
	pinode->i_nr_blocks = inode->orange_inode.i_nr_blocks;
	mark_buff_dirty(bh);
	brelse(bh);
	// sync_buffers(0);
	return 0;
}

PUBLIC void orange_read_inode(struct inode * inode){
	orange_fill_inode(inode, inode->i_sb, inode->i_no);
}

/*****************************************************************************
 *                                new_dir_entry
 *****************************************************************************/
/**
 * Write a new entry into the directory.
 *
 * @param dir_inode  I-node of the directory.
 * @param inode_nr   I-node nr of the new file.
 * @param filename   Filename of the new file.
 *****************************************************************************/
PRIVATE void orange_new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
{
	if(dir_inode->orange_inode.i_nr_blocks == 0){
		orange_alloc_smap_bit(dir_inode->i_sb, &(dir_inode->orange_inode));
	}
	/* write the dir_entry */
	// 原orange逻辑存在问题：当目录项使用至Blk边界时,对于仍有空间的目录没有正确分配空间
	int dir_blk0_nr = dir_inode->orange_inode.i_start_block;
	unsigned int dir_blk_total = dir_inode->orange_inode.i_nr_blocks;
	unsigned int nr_dir_blks = (dir_inode->i_size-1+ BLOCK_SIZE) / BLOCK_SIZE;
	unsigned int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
											 * including unused slots
											 * (the file has been
											 * deleted but the slot
											 * is still there)
											 */
	struct dir_entry *pde;
	struct dir_entry *new_de = 0;

	unsigned int i, j, m = 0;
	char *fsbuf =NULL;
	buf_head *bh = NULL;
	// find free slot in disk
	for (i = 0; i <= nr_dir_blks && i < dir_blk_total; i++)// 此处取等,允许多读取一块
	{
		bh = bread(dir_inode->i_dev, dir_blk0_nr + i);
		fsbuf = bh->buffer;
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < BLOCK_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0)
			{ /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries || new_de)	/* all entries have been iterated or free slot is found */
			break;
		brelse(bh);

	}
	if (!new_de)
	{ /* reached the end of the dir */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);
	new_de->name[strlen(filename)] = 0;

	/* write dir block -- ROOT dir block */
	mark_buff_dirty(bh);
	brelse(bh);
	orange_sync_inode(dir_inode);
}

PUBLIC struct dentry* orange_lookup(struct inode* dir, const char* filename){
	int inode_nr = lookup_inode_in_dir(dir, filename);
	if(inode_nr != INVALID_INODE){
		struct inode * new_inode = vfs_get_inode(dir->i_sb, inode_nr);
		struct dentry * dentry = vfs_new_dentry(filename, new_inode);
		return dentry;
	}
	return NULL;
}

PUBLIC int orange_read(struct file_desc* file, unsigned int count, char* buf){
	int pos = file->fd_pos;
	struct inode *pin = file->fd_dentry->d_inode;
	int pos_end = min(pos + count, pin->i_size);
	if(pin->orange_inode.i_nr_blocks == 0) {
		return 0;
	}
	int off = pos % BLOCK_SIZE;
	int rw_sect_min = pin->orange_inode.i_start_block + (pos >> BLOCK_SIZE_SHIFT);
	int rw_sect_max = pin->orange_inode.i_start_block + (pos_end >> BLOCK_SIZE_SHIFT);
	int chunk = min(rw_sect_max - rw_sect_min + 1,BLOCK_SIZE >> BLOCK_SIZE_SHIFT);
	int bytes_rw = 0;
	int bytes_left = count;
	int i;
	char *fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = rw_sect_min; i <= rw_sect_max; i += chunk){
		/* read/write this amount of bytes every time */
		int bytes = min(bytes_left, chunk * BLOCK_SIZE - off);
		bh = bread(pin->i_dev,i);
		fsbuf = bh->buffer;
		if (pos + bytes > pos_end){ // added by xiaofeng 2021-9-2
			bytes = pos_end - pos;
		}
		memcpy((void *)(buf + bytes_rw), (void*)(fsbuf + off), bytes);
		off = 0;
		bytes_rw += bytes;
		pos += bytes;
		bytes_left -= bytes;
		brelse(bh);
	}
	file->fd_pos = pos;
	return bytes_rw;
}

PUBLIC int orange_write(struct file_desc* file, unsigned int count, const char* buf){
	int pos = file->fd_pos;
	struct inode *pin = file->fd_dentry->d_inode;
	int sync_needed = 0;
	if(pin->orange_inode.i_nr_blocks == 0) {
		orange_alloc_smap_bit(pin->i_sb, &(pin->orange_inode));
		// pin->orange_inode.i_nr_blocks = NR_DEFAULT_FILE_BLOCKS;
		sync_needed = 1;
	}
	int pos_end = min(pos + count, pin->orange_inode.i_nr_blocks * BLOCK_SIZE);
	int off = pos % BLOCK_SIZE;
	int rw_sect_min = pin->orange_inode.i_start_block + (pos >> BLOCK_SIZE_SHIFT);
	int rw_sect_max = pin->orange_inode.i_start_block + (pos_end >> BLOCK_SIZE_SHIFT);
	int chunk = min(rw_sect_max - rw_sect_min + 1,BLOCK_SIZE >> BLOCK_SIZE_SHIFT);
	int bytes_rw = 0;
	int bytes_left = count;
	int i;
	char *fsbuf = NULL;
	buf_head * bh = NULL;
	for (i = rw_sect_min; i <= rw_sect_max; i += chunk){
		/* read/write this amount of bytes every time */
		int bytes = min(bytes_left, chunk * BLOCK_SIZE - off);
		bh = bread(pin->i_dev,i);
		fsbuf = bh->buffer;
		if (pos + bytes > pos_end){ // added by xiaofeng 2021-9-2
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
	if (file->fd_pos > pin->i_size)
	{
		/* update inode::size */
		pin->i_size = file->fd_pos;
		sync_needed = 1;
	}
	if (sync_needed == 1) {
		/* write the updated i-node back to disk */
		orange_sync_inode(pin);
	}
	return bytes_rw;
}

PUBLIC int orange_get_block(struct inode *inode, u32 iblock, int create)
{
	struct super_block *sb = inode->i_sb;
	int phys = -1;
	if(inode->orange_inode.i_nr_blocks == 0) {
		if(create == 0){
			return -1;
		}
		orange_alloc_smap_bit(sb, &(inode->orange_inode));
	}
	if(iblock < inode->orange_inode.i_nr_blocks) {
		phys = inode->orange_inode.i_start_block + iblock;
	}
	return phys;
}

int orange_create(struct inode *dir, struct dentry*dentry, int mode){
	int inode_nr = orange_alloc_imap_bit(dir->i_sb);
	if(inode_nr == INVALID_INODE){
		return -1;
	}
	struct inode *newino = vfs_new_inode(dir->i_sb);
	// int free_sect_nr = orange_alloc_smap_bit(dir->i_sb, NR_DEFAULT_FILE_BLOCKS);
	// create empty file not map sector first to save space
	dentry->d_inode = newino;
	newino->i_no = inode_nr;
	newino->i_sb = dir->i_sb;
	newino->i_dev = dir->i_dev;
	newino->orange_inode.i_start_block = 0;
	newino->orange_inode.i_nr_blocks = 0;
	newino->i_size = 0;
	newino->i_nlink = 1;
	newino->i_mode = I_RWX;
	newino->i_type = I_REGULAR;
	newino->i_op = &orange_inode_ops;
	newino->i_fop = &orange_file_ops;
	orange_new_dir_entry(dir, newino->i_no, dentry_name(dentry));
	orange_sync_inode(newino);
	return 0;
}

int orange_unlink(struct inode *dir, struct dentry*dentry){
	return remove_name_in_dir(dir, dentry->d_inode->i_no);
}

int orange_mkdir(struct inode *dir, struct dentry*dentry, int mode){
	int inode_nr = orange_alloc_imap_bit(dir->i_sb);
	if(inode_nr == INVALID_INODE){
		return -1;
	}
	struct inode *newino = vfs_get_inode(dir->i_sb, inode_nr);
	// int free_sect_nr = orange_alloc_smap_bit(dir->i_sb, NR_DEFAULT_FILE_BLOCKS);
	dentry->d_inode = newino;
	newino->i_no = inode_nr;
	newino->i_sb = dir->i_sb;
	newino->i_dev = dir->i_dev;
	newino->i_size = 0;
	newino->i_nlink = 1;
	newino->orange_inode.i_start_block = 0;
	newino->orange_inode.i_nr_blocks = 0;
	newino->i_mode = I_RWX;
	newino->i_type = I_DIRECTORY;
	newino->i_op = &orange_inode_ops;
	newino->i_fop = &orange_file_ops;
	orange_new_dir_entry(dir, newino->i_no, dentry_name(dentry));
	// orange_new_dir_entry(newino, newino->i_no, ".");
	// orange_new_dir_entry(newino, dir->i_no, "..");
	orange_sync_inode(newino);
	return 0;
}

int orange_rmdir(struct inode *dir, struct dentry*dentry){
	if(orange_check_dir_empty(dentry->d_inode) != 0){
		return -1;
	}
	return remove_name_in_dir(dir, dentry->d_inode->i_no);
}

void orange_deleteinode(struct inode* inode){
	orange_free_smap_bit(inode->i_sb,
		inode->orange_inode.i_start_block,
		inode->orange_inode.i_nr_blocks);
	orange_free_imap_bit(inode->i_sb, inode->i_no);
}

int orange_fill_superblock(struct super_block* sb, int dev){
	buf_head * bh = bread(dev, 1);
	struct orange_sb_info *psb = (struct orange_sb_info*)bh->buffer;
	*(ORANGE_SB(sb)) = *psb;
	sb->sb_dev = dev;
	sb->fs_type = ORANGE_TYPE;
	sb->sb_op = &orange_sb_ops;
	sb->sb_blocksize = num_4K;
	struct inode * orange_root = vfs_get_inode(sb, ORANGE_SB(sb)->root_inode);
	sb->sb_root = vfs_new_dentry("/", orange_root);
	brelse(bh);
	return 0;
}

struct superblock_operations orange_sb_ops = {
.fill_superblock = orange_fill_superblock,
.sync_inode = orange_sync_inode,
.read_inode = orange_read_inode,
.put_inode = NULL,
.delete_inode = orange_deleteinode,
};

struct inode_operations orange_inode_ops = {
.lookup = orange_lookup,
.create = orange_create,
.unlink = orange_unlink,
.mkdir = orange_mkdir,
.rmdir = orange_rmdir,
.get_block = orange_get_block,
};

struct file_operations orange_file_ops = {
#ifdef OPT_PAGE_CACHE
.write = generic_file_write,
.read = generic_file_read,
.fsync = generic_file_fsync,
#else
.write = orange_write,
.read = orange_read,
.fsync = NULL,
#endif
.readdir = orange_readdir,
};
// #endif
