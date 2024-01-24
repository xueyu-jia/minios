/**
 * fs_misc.h
 * This file contains miscellaneous defines and declarations associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

#ifndef	FS_H
#define	FS_H

#include "type.h"
#include "spinlock.h"
#include "fs_const.h"
#include "proc.h"
#include "vfs.h"

/**
 * structure for fs common ***文件系统公共数据结构***
 * 
 * 
*/
#include "orangefs.h"
#include "fat32.h"
typedef struct vfs_inode{
	u32 i_no;
	struct super_block* i_sb;
	u32 i_dev; // for char special inode
	u32 i_nlink; // file linked
	u32 i_count; // reference count
	u32 i_size;  // file size in byte
	int i_type;  // char/blk/mnt/dir...
	int i_mode;  // permission ==> I_R/W/X
	u32 i_mnt_index;
	struct inode_operations *i_op;
	struct file_operations *i_fop;
	union {
		struct orange_inode_info orange_inode;
	};
	struct vfs_inode* lru_nxt;
	struct vfs_inode* lru_pre;
	struct spinlock lock;
}vfs_inode;

// **** dentry 的本质是树形cache !!! ****
struct vfs_dentry{
	char d_name[MAX_DNAME_LEN];
	struct vfs_inode* d_inode;
	struct vfs_dentry* d_nxt;
	struct vfs_dentry* d_pre;
	struct vfs_dentry* d_subdirs;
	struct vfs_dentry* d_parent;
	struct vfs_dentry* d_covers;
	struct vfs_dentry* d_mounts;
	struct dentry_operations * d_op;
	struct spinlock lock;
};
/**
 * @struct super_block fs.h "include/fs.h"
 * @brief  The 2nd sector of the FS
 *
 * Remember to change SUPER_BLOCK_SIZE if the members are changed.
 */
 //modified by sundong 2023.5.26
struct super_block {
	union {
		struct orange_sb_info orange_sb;
		struct fat32_sb_info fat32_sb;
    	struct {
			u32 TotalSectors;//总扇区数，当载入磁盘时，才从DBR中读取。
			u16  Bytes_Per_Sector;//每个扇区的字节数，当载入磁盘时，才从DBR中读取。
			u8  Sectors_Per_Cluster;//每个簇的扇区数，当载入磁盘时，才从DBR中读取。
			u16  Reserved_Sector;//保留扇区数，当载入磁盘时，才从DBR中读取。
			u32 Sectors_Per_FAT;//每个FAT所占的扇区数，当载入磁盘时，才从DBR中读取。
			u32 Position_Of_RootDir;//根目录的位置。
			u32 Position_Of_FAT1;//FAT1的位置。
			u32 Position_Of_FAT2;//FAT2的位置。
    	};
	};

  /*
   * the following item(s) are only present in memory
   */
  	struct vfs_dentry* root;
	struct file_operations * sb_fop;
	struct inode_operations * sb_iop;
	struct super_operations * sb_sop;
	struct dentry_operations * sb_dop;
	int	sb_dev; 	/**< the super block's home device */
	int fs_type;	//added by mingxuan 2020-10-30
	int used;
	SPIN_LOCK lock;
};

/**
 * @struct file_desc
 * @brief  File Descriptor
 */

//added by mingxuan 2019-5-17
// union ptr_node{
// 	struct inode*	fd_inode;	/**< Ptr to the i-node */
// 	//struct FILE* 	fd_file; // modified by ran
// 	struct File* 	fd_file;	//指向fat32的file结构体
// };

struct file_desc {
	int 	flag;	//用于标志描述符是否被使用
	int		fd_mode;	/**< R or W */
	int		fd_pos;		/**< Current position for R/W. */
	struct vfs_dentry *fd_dentry;
};

struct dirent{
	int d_ino;
	char d_name[MAX_DNAME_LEN];
};

// opendir mallock a page and take beginning as dirstream struct, remaining data as dir raw dirent
// | ------4K-------------|
// |dirstream|dirent[]... |
struct dirstream{
	struct file_desc* file;
	int init;
	int count;
	int total;
};
#define DIR_DATA(dirp) ((struct dirent*)((dirp) + 1))

typedef struct dirstream DIR;

struct inode_operations{
	struct vfs_dentry * (*lookup)(struct vfs_inode *dir, char *filename);
	int (*create)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*unlink)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*mkdir)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*rmdir)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*readdir)(struct vfs_inode* dir, struct dirent* start);
	int (*sync_inode)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*put_inode)(struct vfs_inode* inode);
	int (*delete_inode)(struct vfs_inode* inode);
};

struct dentry_operations{
	int (*compare)(const char *dentryname, const char *filename);
};

struct file_operations{
	int (*read)(struct file_desc *file, unsigned int count, char * buf);
	int (*write)(struct file_desc *file, unsigned int count, char * buf);
};

struct superblock_operations{
	int (*fill_superblock)(struct super_block *, int);
};

struct fs{
	char * fstype_name;
	struct file_operations * fs_fop;
	struct inode_operations * fs_iop;
	struct superblock_operations * fs_sop;
	struct dentry_operations * fs_dop;
};

extern struct super_block super_blocks[NR_SUPER_BLOCK];
int get_fs_dev(int drive, u32 fs_type);
int get_free_superblock();


#endif /* FS_MISC_H */
