// /**********************************************************
// *	fat32.c       
// ***********************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "buffer.h"
#include "vfs.h"
#include "fat32.h"

PRIVATE void uni2char(u16* uni_name, char* norm_name, int max_len){
	u16 c;
	for(int i = 0; (c = *(uni_name++)) &&i < max_len; i++){
		*(norm_name++) = c&0xFF;
	}
	*norm_name = 0;
}

PRIVATE struct fat_info* alloc_fat_info(int cluster_start){
	struct fat_info* ent = kern_kmalloc(sizeof(struct fat_info));
	ent->cluster_start = cluster_start;
	ent->length = 1;
	ent->next = NULL;
	return ent;
}

// if *bh not NULL, origin blk
// getblk by sector(512 bytes), return buf pointer and update bh
PRIVATE char* bread_sector(int dev, int sector, buf_head** bh){
	buf_head* b;
	b = bread(dev, sector/SECTOR_PER_BLOCK);
	if(!b) return NULL;
	if(*bh){
		brelse(*bh);
	}
	*bh = b;
	return ((char*)b->buffer) + ((sector&(SECTOR_PER_BLOCK-1)) << SECTOR_SIZE_SHIFT);
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
	struct buf_head *bh = NULL, *bh2 = NULL;
	char* buf = bread_sector(dev, fat_sector, &bh);
	next = ((u32*)(buf))[(fat_offset&(SECTOR_SIZE-1)) >> 2];
	if (next >= 0xffffff7) next = FAT_END;
	if (value != -1){ // write
		((u32*)(buf))[(fat_offset&(SECTOR_SIZE-1)) >> 2] = value;
		mark_buff_dirty(bh);
		// fat copy
		for(int fat = 1; fat < FAT_SB(sb)->fat_num; fat++){
			fat_sector = FAT_SB(sb)->fat_start_sector 
				+ (fat * FAT_SB(sb)->fat_size) + (fat_offset >> SECTOR_SIZE_SHIFT);
			bread_sector(dev, fat_sector, &bh2);
			memcpy(bh2->buffer, bh->buffer, num_4K);
			mark_buff_dirty(bh2);
			brelse(bh2);
		}
	}
	brelse(bh);
	return next;
}

PRIVATE int add_new_cluster(struct super_block* sb, int tail){
	if(FAT_SB(sb)->fsinfo.cluster_free_count <= 0){
		return -1;
	}
	acquire(&FAT_SB(sb)->lock);
	int i, clus, next_free = FAT_SB(sb)->fsinfo.cluster_next_free, 
		total = FAT_SB(sb)->max_cluster - 1;
	buf_head* bh;
	for(i = 0; i < total; i++){// we must check before use,(linux dont use fsinfo.nextfree)
		clus = 2 + (next_free - 2 + i)%total;
		if(read_write_fat(sb, clus, -1) == 0)
			break;
	}
	if(i >= total){
		FAT_SB(sb)->fsinfo.cluster_free_count = 0;
		disp_str("error: no space");
		release(&FAT_SB(sb)->lock);
		return -1;
	}
	read_write_fat(sb, clus, FAT_END);
	FAT_SB(sb)->fsinfo.cluster_free_count--;
	// update fsinfo
	for(i = 2; i <= total + 1; i++){
		if(read_write_fat(sb, i, -1) == 0)
			break;
	}
	FAT_SB(sb)->fsinfo.cluster_next_free = i;
	struct fat_fsinfo* info_buf = (struct fat_fsinfo*)(
		bread_sector(sb->sb_dev, FAT_SB(sb)->fsinfo_sector, &bh) + FS_INFO_SECTOR_OFFSET);
	memcpy(info_buf, &FAT_SB(sb)->fsinfo, sizeof(struct fat_fsinfo));
	mark_buff_dirty(bh);
	brelse(bh);
	if(tail){
		read_write_fat(sb, tail, clus);
	}
	release(&FAT_SB(sb)->lock);
	return clus;
}


PRIVATE int fill_fat_info(struct super_block* sb, int cluster_start, struct fat_info** info){
	struct fat_info* fat_head = alloc_fat_info(cluster_start);
	struct fat_info* fat_tail = fat_head, *fat_ent = NULL;
	int cluster = cluster_start, next, count = 0;
	*info = fat_head;
	if(!cluster){
		return 0; // according to FAT doc, cluster in entry will be 0 for empty file;
	}
	acquire(&FAT_SB(sb)->lock);
	while(cluster != FAT_END){
		next = read_write_fat(sb, cluster, -1);
		if(next == cluster + 1){
			fat_tail->length++;
		}else if(next != FAT_END){
			fat_ent = alloc_fat_info(next);
			fat_tail->next = fat_ent;
			fat_tail = fat_ent;
		}
		cluster = next;
		count++;
	}
	release(&FAT_SB(sb)->lock);
	return count;
}

// calculate sector num, if new_space set and off over size, alloc new cluster and update fat 
PRIVATE int fat_get_sector(struct vfs_inode* inode, int off, int new_space){
	struct super_block* sb = inode->i_sb;
	int clus_sector = off >> SECTOR_SIZE_SHIFT;
	int clus_skip = clus_sector / FAT_SB(sb)->cluster_sector;
	int clus, last;
	struct fat_info* info = inode->fat32_inode.fat_info;
	if(!info->cluster_start){
		return -1;
	}
	while(clus_skip && info){
		if(clus_skip < info->length)
			break;
		if(!info->next && new_space){
			last = info->cluster_start + info->length - 1;
			clus = add_new_cluster(sb, last);
			if(!last){
				info->cluster_start = clus;
				continue;
			}
			if(clus == last + 1){
				info->length++;
				continue;
			}else{// new alloc cluster not consecutive
				info->next = alloc_fat_info(clus);
			}
		}
		clus_skip -= info->length;
		info = info->next;
	}
	if(!info){
		return -1;
	}
	clus_sector = FAT_SB(sb)->data_start_sector 
		+(info->cluster_start+clus_skip -2)* FAT_SB(sb)->cluster_sector + (clus_sector%FAT_SB(sb)->cluster_sector);
	return clus_sector;
}

PRIVATE char* fat_get_data(struct vfs_inode* inode, int off, buf_head** bh, int alloc_new){
	int clus_sector = fat_get_sector(inode, off, alloc_new);
	if(clus_sector == -1){
		return NULL;
	}
	return bread_sector(inode->i_dev, clus_sector, bh) + (off & (SECTOR_SIZE-1));
}

PRIVATE struct fat_dir_slot* fat_get_slot(struct vfs_inode* dir, int order, buf_head** bh, int alloc_new){
	return (struct fat_dir_slot*)fat_get_data(dir, (order)*FAT_ENTRY_SIZE, bh, alloc_new);
}

PRIVATE struct fat_dir_entry* fat_get_entry(struct vfs_inode* dir, int* start, buf_head** bh, char* full_name){
	if(!start){
		return NULL;
	}
	struct fat_dir_entry* de = NULL;
	struct fat_dir_slot* ds;
	int n_slot = 0, offset, is_long = 0;
	u8  chksum;
	u16 uni_name[256];
	for(;(*start)*FAT_ENTRY_SIZE < dir->i_size; (*start)++){
		ds = fat_get_slot(dir, *start, bh, 0);
		if(!ds)break;
		if(ds->order == 0){
			break;
		}
		if(ds->order == DIR_DELETE){
			is_long = 0;
			continue;
		}
		if( ds->attr == ATTR_LNAME ){
			if(ds->order & DIR_LDIR_END){
				n_slot = ds->order & (~DIR_LDIR_END);
				chksum = ds->checksum;
				is_long = 1;
			}
			if((n_slot != (ds->order & (~DIR_LDIR_END))) || chksum != ds->checksum){
				disp_str("error: dismatch slot");
				is_long = 0;
				continue;// ignore invalid slot
			}
			offset = (n_slot-1)*13;
			memcpy(&uni_name[offset], ds->name0_4, 10);
			offset += 5;
			memcpy(&uni_name[offset], ds->name5_10, 12);
			offset += 6;
			memcpy(&uni_name[offset], ds->name11_12, 4);
			offset += 2;
			if(ds->order & DIR_LDIR_END){
				uni_name[offset] = 0;
			}
			n_slot--;
		}else if(!(ds->attr&ATTR_VOL)){// skip volume
			// check sum
			u8 sum;
			int i;
			de = (struct fat_dir_entry*)ds;
			for (sum = 0, i = 0; i < 11; i++) {
				sum = (((sum&1)<<7)|((sum&0xfe)>>1)) + de->name[i]; 
				//两部分的name实际是连续的，所以没问题
			}
			if(sum != chksum){
				disp_str("error: chksum wrong");
				is_long = 0;
				continue;
			}
			if(is_long){
				uni2char(uni_name, full_name, 256);
			}else{
				memcpy(full_name, de->name, 8);
				full_name[8] = '.';
				memcpy(full_name + 9, de->ext, 3);
				full_name[12] = 0;
			}
			break;
		}
	}
	return de;
}

PRIVATE int fat_find_free(struct vfs_inode* dir, int num){
	buf_head* bh;
	struct fat_dir_slot* ds;
	int start, count = 0, res = -1;
	for(start = 0; ; start++){
		ds = fat_get_slot(dir, start, &bh, 1);
		if(ds->order == DIR_DELETE){
			if(count == 0)res = start;
			count++;
			if(count >= num)break;
		}else if(ds->order == 0){
			res = start;
			break;// 后面的均为空闲，不用找了
		}
		count = 0;
	}
	return res;
}

PRIVATE int fat_check_short(struct vfs_inode* dir, const char* name){

}

PRIVATE void fat_add_name(struct vfs_inode* dir, const char* name, int clus_start){

}

PUBLIC void fat32_read_inode(struct vfs_inode* inode){
	struct super_block* sb = inode->i_sb;
	buf_head* bh = NULL;
	struct fat_dir_entry* entry = NULL;
	int cluster_start;
	int ino = inode->i_no;
	if(ino == FAT_ROOT_INO){
		cluster_start = FAT_SB(sb)->root_cluster;
		inode->i_mode = I_RWX;
		inode->i_type = I_DIRECTORY;
	}else{
		entry = &((struct fat_dir_entry*)bread_sector(sb->sb_dev, ino >> FAT_DPS_SHIFT, &bh))[ino&(FAT_DPS-1)];
		cluster_start = entry->start_l | (entry->start_h << 16);
		inode->i_mode = (entry->attr & ATTR_RO)? I_R|I_X : I_RWX;
		inode->i_type = (entry->attr & ATTR_DIR)? I_DIRECTORY: I_REGULAR;
		inode->i_size = entry->size;
	}
	inode->i_sb = sb;
	inode->i_dev = sb->sb_dev;
	inode->i_no = ino;
	int cnt = fill_fat_info(sb, cluster_start, &inode->fat32_inode.fat_info);
	if(!inode->i_size){
		inode->i_size = cnt * FAT_SB(sb)->cluster_sector * SECTOR_SIZE;
	}
	inode->i_nlink = 1;
	inode->i_op = sb->sb_iop;
	inode->i_fop = sb->sb_fop;
	if(bh) brelse(bh);
	return inode;
}

PUBLIC int fat32_sync_inode(struct vfs_inode* inode){
	if(inode->i_no == FAT_ROOT_INO){
		return 0;
	}
	buf_head* bh;
	int ino = inode->i_no;
	int start = inode->fat32_inode.fat_info->cluster_start;
	struct fat_dir_entry* de = 
		&((struct fat_dir_entry*)bread_sector(inode->i_dev, ino >> FAT_DPS_SHIFT, &bh))[ino&(FAT_DPS-1)];
	if(!de){
		return -1;
	}
	de->size = inode->i_size;
	if(!((de->start_h<<16)|de->start_l)&& start){
		de->start_h = start >> 16;
		de->start_l = start & 0xFFFF;
	}
	if(bh){
		mark_buff_dirty(bh);
		brelse(bh);
	}
	return 0;
}

PUBLIC void fat32_put_inode(struct vfs_inode* inode){
	struct fat_info* next;
	for(struct fat_info* ent=inode->fat32_inode.fat_info; ent; ent = next){
		next = ent->next;
		kern_kfree(ent);
	}
}

PUBLIC struct vfs_dentry* fat32_lookup(struct vfs_inode* dir, const char* filename){
	// 1. format
	// 2. loop get entry until match
	char full_name[256]; // for long dir store real name
	int entry = 0, ino = 0;
	struct fat_dir_entry* de;
	buf_head* bh;
	struct vfs_dentry* dentry = NULL;
	while(entry* FAT_ENTRY_SIZE < dir->i_size){
		de = fat_get_entry(dir, &entry, &bh, full_name);
		if(!de)break;
		if(!stricmp(filename, full_name)){
			ino = (fat_get_sector(dir, entry* FAT_ENTRY_SIZE, 0) << FAT_DPS_SHIFT)
				 | (entry&(FAT_DPS-1));
			break;
		}
	}
	if(bh){
		brelse(bh);
	}
	if(ino){
		struct vfs_inode * inode = vfs_get_inode(dir->i_sb, ino);
		dentry = new_dentry(full_name, inode);
		dentry->d_op = &fat32_dentry_ops;
	}
	return dentry;
}

PUBLIC int fat32_create(struct vfs_inode* dir, struct vfs_dentry* dentry){

}

PUBLIC int fat32_read(struct file_desc* file, unsigned int count, char* buf){
	struct vfs_inode* inode = file->fd_dentry->d_inode;
	int start, pos, end, len;
	buf_head* bh;
	char* file_buf;
	start = file->fd_pos;
	end = min(start + count, inode->i_size);
	for(pos = start; pos < end;){
		len = min(SECTOR_SIZE - (pos&(SECTOR_SIZE-1)), end - pos);
		file_buf = fat_get_data(inode, pos, &bh, 0);
		if(!file_buf){
			break;
		}
		memcpy(buf + pos - start, file_buf, len);
		pos += len;
	}
	if(bh){
		brelse(bh);
	}
	file->fd_pos = pos;
	return pos - start;
}

PUBLIC int fat32_write(struct file_desc* file, unsigned int count, char* buf){
	struct vfs_inode* inode = file->fd_dentry->d_inode;
	int start, pos, end, len;
	buf_head* bh;
	char* file_buf;
	start = file->fd_pos;
	end = min(start + count, inode->i_size);
	for(pos = start; pos < end;){
		len = min(SECTOR_SIZE - (pos&(SECTOR_SIZE-1)), end - pos);
		file_buf = fat_get_data(inode, pos, &bh, 1);
		if(!file_buf){
			break;
		}
		memcpy(file_buf, buf + pos - start, len);
		mark_buff_dirty(bh);
		pos += len;
	}
	if(bh){
		brelse(bh);
	}
	file->fd_pos = pos;
	return pos - start;
}

PUBLIC int fat32_readdir(struct file_desc* file, struct dirent* start){
	char full_name[256]; // for long dir store real name
	int entry = 0, count = 0;
	struct fat_dir_entry* de;
	buf_head* bh;
	struct vfs_inode* dir = file->fd_dentry->d_inode;
	struct vfs_dentry* dentry = NULL;
	if(dir->i_no == FAT_ROOT_INO){
		start->d_ino = FAT_ROOT_INO;
		strcpy(start->d_name, ".");
		start++;
		start->d_ino = FAT_ROOT_INO;
		strcpy(start->d_name, "..");
		start++;
	}
	while(entry* FAT_ENTRY_SIZE < dir->i_size){
		de = fat_get_entry(dir, &entry, &bh, full_name);
		if(!de)break;
		start->d_ino = (fat_get_sector(dir, entry* FAT_ENTRY_SIZE, 0) << FAT_DPS_SHIFT)
			| (entry&(FAT_DPS-1));
		strcpy(start->d_name, full_name);
		start++;
		count++;
	}
	if(bh){
		brelse(bh);
	}
	return count;
}

PUBLIC int fat32_fill_superblock(struct super_block* sb, int dev){
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
	FAT_SB(sb)->root_cluster = bpb->RootClus;
	FAT_SB(sb)->fsinfo_sector = bpb->FSInfo * sector_size_mult;
	fsinfo_blk = FAT_SB(sb)->fsinfo_sector/SECTOR_PER_BLOCK;
	if(fsinfo_blk){
		fs_info_buf = bread(dev, fsinfo_blk);
	}else{
		fs_info_buf = bh;
	}
	FAT_SB(sb)->fsinfo = *((struct fat_fsinfo*)(
		(char*)fs_info_buf->buffer + (FAT_SB(sb)->fsinfo_sector%SECTOR_PER_BLOCK)*SECTOR_SIZE
		 + FS_INFO_SECTOR_OFFSET));
	if(FAT_SB(sb)->fsinfo.cluster_next_free == -1){// may invalid(-1)
		FAT_SB(sb)->fsinfo.cluster_next_free = 2;
	}
	if(fsinfo_blk){
		brelse(fs_info_buf);
	}
	initlock(&FAT_SB(sb)->lock, "FAT");
	sb->sb_dev = dev;
	sb->fs_type = FAT32_TYPE;
	sb->sb_iop = &fat32_inode_ops;
	sb->sb_fop = &fat32_file_ops;
	sb->sb_dop = &fat32_dentry_ops;
	sb->sb_sop = &fat32_sb_ops;
	struct vfs_inode * fat32_root = vfs_get_inode(sb, FAT_ROOT_INO);
	sb->sb_root = new_dentry("/", fat32_root);
	sb->sb_root->d_op = &fat32_dentry_ops;
	brelse(bh);
	return 0;
}


struct superblock_operations fat32_sb_ops = {
.fill_superblock = fat32_fill_superblock,
.read_inode = fat32_read_inode,
};

struct inode_operations fat32_inode_ops = {
.lookup = fat32_lookup,
.create = fat32_create,
.put_inode = fat32_put_inode,
};

struct dentry_operations fat32_dentry_ops = {
.compare = stricmp,
};

struct file_operations fat32_file_ops = {
.read = fat32_read,
.write = fat32_write,
.readdir = fat32_readdir,
};