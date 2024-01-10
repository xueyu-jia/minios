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
	u32 i_count; // reference count
	u32 i_size;  // file size in byte
	u32 i_nlink; // file refered by inode
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
	char d_name[32];
	struct vfs_inode* d_inode;
	struct vfs_dentry* d_nxt;
	struct vfs_dentry* d_pre;
	struct vfs_dentry* d_subdirs;
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
    	struct {
			u32	magic;		  /**< Magic number */
			u32	nr_inodes;	  /**< How many inodes */
			u32	nr_blocks;	  /**< How many blocks */
			u32	nr_imap_blocks;	  /**< How many inode-map blocks */
			u32	nr_smap_blocks;	  /**< How many sector-map blocks */
			u32	n_1st_block;	  /**< Number of the 1st data block */
			u32	nr_inode_blocks;   /**< How many inode blocks */
			u32	root_inode;       /**< Inode nr of root directory */
			u32	inode_size;       /**< INODE_SIZE */
			u32	inode_isize_off;  /**< Offset of `struct inode::i_size' */
			u32	inode_start_off;  /**< Offset of `struct inode::i_start_sect' */
			u32	dir_ent_size;     /**< DIR_ENTRY_SIZE */
			u32	dir_ent_inode_off;/**< Offset of `struct dir_entry::inode_nr' */
			u32	dir_ent_fname_off;/**< Offset of `struct dir_entry::name' */
    	};
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
union ptr_node{
	struct inode*	fd_inode;	/**< Ptr to the i-node */
	//struct FILE* 	fd_file; // modified by ran
	struct File* 	fd_file;	//指向fat32的file结构体
};

struct file_desc {
	int		fd_mode;	/**< R or W */
	int		fd_pos;		/**< Current position for R/W. */
	//struct inode*	fd_inode;	/**< Ptr to the i-node */	//deleted by mingxuan 2019-5-17
	
	//added by mingxuan 2019-5-17
	union 	ptr_node fd_node;
	struct vfs_inode *fd_inode;
	int 	flag;	//用于标志描述符是否被使用
	int 	dev_index;
};

struct inode_operations{
	struct vfs_dentry * (*lookup)(struct vfs_inode *dir, char *filename);
	int (*create)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*unlink)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*mkdir)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*rmdir)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*rename)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*readdir)(struct vfs_inode *dir);
	int (*put_inode)(struct vfs_inode* inode);
};

struct dentry_operations{
	int (*compare)(struct vfs_dentry *dentry, char *filename);
};

struct file_operations{
	int (*read)(struct file_desc *, unsigned int, char *);
	int (*write)(struct file_desc *, unsigned int, char *);
};

struct superblock_operations{
	int (*fill_superblock)(struct super_block *, int);
};

struct fs{
	char * fstype_name;
	struct file_operations * fs_fop;
	struct inode_operations * fs_iop;
	struct super_operations * fs_sop;
	struct dentry_operations * fs_dop;
};

//added by mingxuan 2020-10-29
struct vfs{
    char * fs_name; 			//设备名
    struct file_op * op;        //指向操作表的一项
	struct sb_op *s_op;			//added by mingxuan 2020-10-29
    //int  dev_num;             //设备号	//deleted by mingxuan 2020-10-29
	struct file_operations * fs_fop;
	struct inode_operations * fs_iop;
	struct super_operations * fs_sop;
	struct dentry_operations * fs_dop;
	struct super_block *sb;		//added by mingxuan 2020-10-29
	int used;                   //added by ran
};

int get_fs_dev(int drive, int fs_type);



#endif /* FS_MISC_H */
