// /**********************************************************
// *	fat32.c       
// ***********************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "buffer.h"
#include "vfs.h"
#include "fat32.h"

PRIVATE struct fat_ent* alloc_fat_ent(int cluster_start){
	struct fat_ent* ent = kern_kmalloc(sizeof(struct fat_ent));
	ent->cluster_start = cluster_start;
	ent->length = 1;
	ent->next = NULL;
	return ent;
}

// value == -1 for read
PRIVATE int read_write_fat(struct super_block* sb, int cluster, int value){
	int dev = sb->sb_dev, next;
	if(cluster > FAT_SB(sb)->max_cluster){
		return 0;
	}
	int fat_offset = cluster * 4; 
	// only support FAT32, other implements should judge FAT_SB(sb)->fat_bit
	int fat_sector = FAT_SB(sb)->fat_start_sector + (fat_offset >> SECTOR_SIZE_SHIFT);
	struct buf_head *bh = bread(dev, fat_sector/SECTOR_PER_BLOCK), *bh2 = NULL;
	char* buf = ((char*)bh->buffer) + (fat_sector&(SECTOR_PER_BLOCK-1))*SECTOR_SIZE;
	next = ((u32*)(buf))[(fat_offset&(SECTOR_SIZE-1)) >> 2];
	if (next >= 0xffffff7) next = FAT_END;
	if (value != -1){ // write
		((u32*)(buf))[(fat_offset&(SECTOR_SIZE-1)) >> 2] = value;
		mark_buff_dirty(bh);
		// fat copy
		for(int fat = 1; fat < FAT_SB(sb)->fat_num; fat++){
			fat_sector = FAT_SB(sb)->fat_start_sector 
				+ (fat * FAT_SB(sb)->fat_size) + (fat_offset >> SECTOR_SIZE_SHIFT);
			bh2 = bread(dev, fat_sector/SECTOR_PER_BLOCK);
			memcpy(bh2->buffer, bh->buffer, num_4K);
			mark_buff_dirty(bh2);
			brelse(bh2);
		}
	}
	brelse(bh);
	return next;
}

PRIVATE struct fat_ent* fill_fat_ent(struct super_block* sb, int cluster_start){
	struct fat_ent* fat_head = alloc_fat_ent(cluster_start);
	struct fat_ent* fat_tail = fat_head, *fat_ent = NULL;
	int cluster = cluster_start, next;
	while(cluster != FAT_END){
		next = read_write_fat(sb, cluster, -1);
		if(next == cluster + 1){
			fat_tail->length++;
		}else if(next != FAT_END){
			fat_ent = alloc_fat_ent(next);
			fat_tail->next = fat_ent;
			fat_tail = fat_ent;
		}
		cluster = next;
	}
	return fat_head;
}

PUBLIC struct vfs_inode* fat32_get_inode(struct super_block* sb, int cluster_start, int type){
	struct vfs_inode* inode = vfs_get_inode();
	inode->i_sb = sb;
	inode->i_dev = sb->sb_dev;
	inode->i_no = cluster_start - 1;
	inode->fat32_inode.fat_info = fill_fat_ent(sb, cluster_start);
	inode->i_type = type;
	inode->i_nlink = 1;
	inode->i_mode = I_RWX;
	inode->i_op = sb->sb_iop;
	inode->i_fop = sb->sb_fop;
	return inode;
}

PUBLIC void fat32_put_inode(struct vfs_inode* inode){
	struct fat_ent* next;
	for(struct fat_ent* ent=inode->fat32_inode.fat_info; ent; ent = next){
		next = ent->next;
		kern_kfree(ent);
	}
}

int fat32_fill_superblock(struct super_block* sb, int dev){
	int dir_sectors, fsinfo_blk, ext_flag;
	buf_head * fs_info_buf;
	buf_head * bh = bread(dev, 0);
	struct fat32_bpb* bpb  = (struct fat32_bpb*)bh->buffer;
	int sector_size_mult = bpb->BytsPerSec >> SECTOR_SIZE_SHIFT;
	if(bpb->FATSz16 != 0){
		// not FAT32 volume, current not supported
		brelse(bh);
		return -1;
	}else{
		FAT_SB(sb)->fat_bit = 32;
		FAT_SB(sb)->fat_size = bpb->FATSz32 * sector_size_mult;
	}
	if(bpb->TotSec16 != 0){
		FAT_SB(sb)->tot_sector = bpb->TotSec16 * sector_size_mult;
	}else{
		FAT_SB(sb)->tot_sector = bpb->TotSec32 * sector_size_mult;
	}
	FAT_SB(sb)->fat_num = bpb->NumFATs;
	FAT_SB(sb)->cluster_sector = bpb->SecPerClus * sector_size_mult;
	FAT_SB(sb)->fat_start_sector = bpb->RsvdSecCnt * sector_size_mult;
	FAT_SB(sb)->dir_start_sector = FAT_SB(sb)->fat_start_sector + bpb->NumFATs * FAT_SB(sb)->fat_size;
	dir_sectors = (((bpb->RootEntCnt*32) + bpb->BytsPerSec -1 )/ bpb->BytsPerSec) * sector_size_mult;
	// for FAT32, dir_sectors should be 0
	FAT_SB(sb)->data_start_sector = FAT_SB(sb)->dir_start_sector + dir_sectors;
	FAT_SB(sb)->max_cluster = 
		(FAT_SB(sb)->tot_sector - FAT_SB(sb)->data_start_sector)/FAT_SB(sb)->cluster_sector + 1;
	FAT_SB(sb)->fsinfo_sector = bpb->FSInfo * sector_size_mult;
	fsinfo_blk = FAT_SB(sb)->fsinfo_sector/8;
	if(fsinfo_blk){
		fs_info_buf = bread(dev, fsinfo_blk);
	}else{
		fs_info_buf = bh;
	}
	FAT_SB(sb)->fsinfo = *((struct fat32_fsinfo*)(
		(char*)fs_info_buf->buffer + (FAT_SB(sb)->fsinfo_sector%8)*SECTOR_SIZE +  FS_INFO_SECTOR_OFFSET));
	if(fsinfo_blk){
		brelse(fs_info_buf);
	}
	sb->sb_dev = dev;
	sb->fs_type = FAT32_TYPE;
	sb->sb_iop = &fat32_inode_ops;
	sb->sb_fop = &fat32_file_ops;
	sb->sb_dop = &fat32_dentry_ops;
	sb->sb_sop = &fat32_sb_ops;
	struct vfs_inode * fat32_root = fat32_get_inode(sb, bpb->RootClus, I_DIRECTORY);
	sb->root = new_dentry("/", fat32_root);
	brelse(bh);
	return 0;
}


struct superblock_operations fat32_sb_ops = {
.fill_superblock = fat32_fill_superblock,
};

struct inode_operations fat32_inode_ops = {

};

struct dentry_operations fat32_dentry_ops = {

};

struct file_operations fat32_file_ops = {

};