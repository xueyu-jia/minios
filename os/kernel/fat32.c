// /**********************************************************
// *	fat32.c       
// ***********************************************************/

#include "type.h"
#include "const.h"
#include "buffer.h"
#include "vfs.h"
#include "fat32.h"



int fat32_fill_superblock(struct super_block* sb, int dev){
	int dir_sectors, fsinfo_blk;
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
	FAT_SB(sb)->fat_num = bpb->NumFATs;
	FAT_SB(sb)->cluster_sector = bpb->SecPerClus * sector_size_mult;
	FAT_SB(sb)->fat_start_sector = bpb->RsvdSecCnt * sector_size_mult;
	FAT_SB(sb)->dir_start_sector = FAT_SB(sb)->fat_start_sector + bpb->NumFATs * FAT_SB(sb)->fat_size;
	dir_sectors = (((bpb->RootEntCnt*32) + bpb->BytsPerSec -1 )/ bpb->BytsPerSec) * sector_size_mult;
	// for FAT32, dir_sectors should be 0
	FAT_SB(sb)->data_start_sector = FAT_SB(sb)->dir_start_sector + dir_sectors;
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
	struct vfs_inode * fat32_root = vfs_get_inode();
	// orange_fill_inode(orange_root, sb, ORANGE_SB(sb)->root_inode);
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