/**
 * fs_misc.h
 * This file contains miscellaneous defines and declarations associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

#ifndef	FS_H
#define	FS_H

#include "type.h"
#include "atomic.h"
#include "list.h"
#include "spinlock.h"
#include "fs_const.h"
#include "proc.h"

/**
 * structure for fs common ***文件系统公共数据结构***
 * 
 * 
*/
#include "orangefs.h"
#include "fat32.h"
struct vfs_inode{
	u32 i_no;
	struct super_block* i_sb;
	u32 i_rdev;   // real device
	u32 i_b_cdev; // for char special inode
	u32 i_nlink; // file linked
	atomic_t i_count; // reference count
	u32 i_size;  // file size in byte
	int i_type;  // char/blk/mnt/dir...
	int i_mode;  // permission ==> I_R/W/X
	u32 i_atime; // access time (use UTC timestamp, 下同)
	u32	i_ctime; // create time
	u32 i_mtime; // modify time
	// list_head i_dentry; // connected dentry list，待定
	struct list_node i_list; // recently used inodes list
	struct inode_operations *i_op;
	struct file_operations *i_fop;
	struct spinlock lock;
	union {
		struct orange_inode_info orange_inode;
		struct fat32_inode_info fat32_inode;
	};
};

// **** dentry 的本质是cache !!! ****
struct vfs_dentry{
	atomic_t d_count;
	char d_name[MAX_DNAME_LEN];
	struct vfs_inode* d_inode;
	struct vfs_dentry* d_nxt;
	struct vfs_dentry* d_pre;
	struct vfs_dentry* d_subdirs;
	struct vfs_dentry* d_parent;
	struct vfs_mount* d_vfsmount;
	int d_mounted;
	struct dentry_operations * d_op;
	struct spinlock lock;
};
/**
 * @struct super_block fs.h "include/fs.h"
 *
 */
 //modified by sundong 2023.5.26
struct super_block {
	union {
		struct orange_sb_info orange_sb;
		struct fat32_sb_info fat32_sb;
	};

  /*
   * the following item(s) are only present in memory
   */
  	struct vfs_dentry* sb_root;
	list_head sb_inode_list;
	struct superblock_operations * sb_op;
	int	sb_dev; 	/**< the super block's home device */
	int fs_type;	//added by mingxuan 2020-10-30
	int used;
	SPIN_LOCK lock;
};

struct file_desc {
	int 	flag;	//用于标志描述符是否被使用
	int		fd_mode;	/**< R or W */
	atomic_t	fd_count;	//用于维护进程间相同File引用的资源释放
	int		fd_pos;		/**< Current position for R/W. */
	struct vfs_dentry *fd_dentry;
	struct file_operations* fd_ops;
};

#define fget(file) atomic_inc(&((file)->fd_count))

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
	struct vfs_dentry * (*lookup)(struct vfs_inode *dir, const char *filename);
	int (*create)(struct vfs_inode *dir, struct vfs_dentry *dentry, int mode);
	int (*unlink)(struct vfs_inode *dir, struct vfs_dentry *dentry);
	int (*mkdir)(struct vfs_inode *dir, struct vfs_dentry *dentry, int mode);
	int (*rmdir)(struct vfs_inode *dir, struct vfs_dentry *dentry);
};

struct dentry_operations{
	int (*compare)(const char *dentryname, const char *filename);
};

struct file_operations{
	int (*read)(struct file_desc *file, unsigned int count, char * buf);
	int (*write)(struct file_desc *file, unsigned int count, const char * buf);
	int (*readdir)(struct file_desc *file, unsigned int count, struct dirent* start);
};

struct superblock_operations{
	int (*fill_superblock)(struct super_block *, int);
	int (*write_inode)(struct vfs_inode *inode);
	void (*read_inode)(struct vfs_inode *inode);
	void (*put_inode)(struct vfs_inode* inode);
	void (*delete_inode)(struct vfs_inode* inode);
};

struct fs_type{
	char * fstype_name;
	struct file_operations * fs_fop;
	struct inode_operations * fs_iop;
	struct superblock_operations * fs_sop;
	struct dentry_operations * fs_dop;
};

extern struct super_block super_blocks[NR_SUPER_BLOCK];
int get_fs_part_dev(int drive, int part, u32 fs_type);
int get_fs_dev(int drive, u32 fs_type);
int get_free_superblock();


#endif /* FS_MISC_H */
