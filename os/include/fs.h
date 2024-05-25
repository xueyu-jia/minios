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
// forward declarations
struct inode;
struct address_space;
struct dentry;
struct file_desc;
struct super_block;
#include "spinlock.h"
#include "fs_const.h"
#include "mempage.h"

/**
 * structure for fs common ***文件系统公共数据结构***
 * 
 * 
*/


#include "orangefs.h"
#include "fat32.h"

struct address_space {
	struct inode *host;
	int type;
	list_head page_list;
	struct spinlock lock;
};
struct inode {
	u32 i_no;
	struct super_block* i_sb;
	u32 i_dev;   // real device
	u32 i_b_cdev; // for char special inode
	u32 i_nlink; // file linked
	atomic_t i_count; // reference count
	u64 i_size;  // file size in byte
	int i_type;  // char/blk/mnt/dir...
	int i_mode;  // permission ==> I_R/W/X
	u32 i_atime; // access time (use UTC timestamp, 下同)
	u32	i_crtime; // create time
	u32 i_mtime; // modify time
	list_head i_dentry; // connected dentry list，待定
	struct list_node i_list; // recently used inodes list
	// contains inode cached pages, ordered by page offset
	struct address_space *i_mapping;	
	struct address_space i_data;
	struct inode_operations *i_op;
	struct file_operations *i_fop;
	struct spinlock lock;
	union {
		struct orange_inode_info orange_inode;
		struct fat32_inode_info fat32_inode;
	};
};


// **** dentry 的本质是cache !!! ****
struct dentry{
	atomic_t d_count;
	// 对于一般的文件，直接使用d_shortname即可，过长的文件名另单独分配内存
	char d_shortname[MAX_DNAME_LEN];
	char *d_longname;
	struct inode* d_inode;
	struct list_node d_alias;
	struct list_node d_list;
	list_head d_subdirs;
	struct dentry* d_parent;
	struct vfs_mount* d_vfsmount;
	int d_mounted;
	struct dentry_operations * d_op;
	struct spinlock lock;
};
#define dentry_name(d) ((d)->d_longname?(d)->d_longname:(d)->d_shortname)
/**
 * @struct super_block fs.h "include/fs.h"
 *
 */
 //modified by sundong 2023.5.26
struct super_block {
  	struct dentry* sb_root; // 根dentry
	struct vfs_mount* sb_vfsmount; // 
	list_head sb_inode_list;
	struct superblock_operations * sb_op;
	int sb_blocksize;
	int	sb_dev; 	/**< the super block's home device */
	int fs_type;	//added by mingxuan 2020-10-30
	int used;
	struct spinlock lock;
	union {
		struct orange_sb_info orange_sb;
		struct fat32_sb_info fat32_sb;
	};
	// 实际文件系统的元数据
};

struct file_desc {
	int 	flag;	//用于标志描述符是否被使用
	int		fd_mode;	/**< R or W */
	atomic_t	fd_count;	//用于维护进程间相同File引用的资源释放
	u64		fd_pos;		/**< Current position for R/W. */
	struct dentry *fd_dentry;
	struct address_space *fd_mapping;
	struct file_operations* fd_ops;
};

#define fget(file) atomic_inc(&((file)->fd_count))
void fput(struct file_desc* file);

struct dirent{
	int d_len;// total len, including full d_name
	int d_ino;
	char d_name[1]; //just d_name start
};
#define dirent_next(dent) ((struct dirent*)(((char*)dent)+(dent->d_len)))
#define dirent_len(name_len) ((u32)(name_len) + 9) // 2*int+end'\0'
#define dirent_fill(dent, ino, name_len) do{	\
	(dent)->d_ino = ino;\
	(dent)->d_len = dirent_len(name_len);\
}while(0)
// opendir mallock a page and take beginning as dirstream struct, remaining data as dir raw dirent
// | ------4K-------------|
// |dirstream|dirent... |
struct dirstream{
	struct file_desc* file;
	int init;
	int pos;
	int total;
};
#define DIR_DATA(dirp) ((struct dirent*)((dirp) + 1))

typedef struct dirstream DIR;

struct inode_operations{
	struct dentry * (*lookup)(struct inode *dir, const char *filename);
	int (*create)(struct inode *dir, struct dentry *dentry, int mode);
	int (*unlink)(struct inode *dir, struct dentry *dentry);
	int (*mkdir)(struct inode *dir, struct dentry *dentry, int mode);
	int (*rmdir)(struct inode *dir, struct dentry *dentry);
	int (*get_block)(struct inode *inode, u32 iblock, int create);
};

struct dentry_operations{
	int (*compare)(const char *dentryname, const char *filename);
};

struct file_operations{
	int (*read)(struct file_desc *file, unsigned int count, char * buf);
	int (*write)(struct file_desc *file, unsigned int count, const char * buf);
	int (*readdir)(struct file_desc *file, unsigned int count, struct dirent* start);
	int (*fsync)(struct file_desc *file, int datasync);
};

struct superblock_operations{
	int (*fill_superblock)(struct super_block *, int);
	int (*write_inode)(struct inode *inode);
	void (*read_inode)(struct inode *inode);
	void (*put_inode)(struct inode* inode);
	void (*delete_inode)(struct inode* inode);
};

struct fs_type{
	char * fstype_name;
	struct file_operations * fs_fop;
	struct inode_operations * fs_iop;
	struct superblock_operations * fs_sop;
	struct dentry_operations * fs_dop;
	int (*identify)(int drive, u32 start_sector);
};

extern struct super_block super_blocks[NR_SUPER_BLOCK];
extern struct fs_type fstype_table[NR_FS_TYPE];
int get_free_superblock();
int generic_file_readpage(struct address_space* file_mapping, struct page* target);
int generic_file_writepage(struct address_space* file_mapping, struct page* target);
int generic_file_read(struct file_desc* file, unsigned int count, char* buf);
int generic_file_write(struct file_desc* file, unsigned int count, const char* buf);
int generic_file_fsync(struct file_desc* file, int datasync);
#endif /* FS_MISC_H */
