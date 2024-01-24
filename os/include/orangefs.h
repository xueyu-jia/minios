/**
 * fs.h
 * This file contains APIs of filesystem, it's used inside the kernel.
 * There is a seperate header file for user program's use. 
 * This file is added by xw. 18/6/17
 */

#ifndef	FS_ORANGE_H
#define	FS_ORANGE_H
#include "type.h"
#include "const.h"
#include "vfs.h"
/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define	MAGIC_V1	0x111
// #define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */
//add by sundong 2023.5.26
#define NR_DEFAULT_FILE_BLOCKS	256

// mark 路径最大长度
// #define	MAX_PATH	128 common macro in fs_const.h
#define	MAX_FILENAME_LEN	12


/* Error types of dir option*/
#define DIR_PATH_INEXISTE -1
#define DIR_PATH_REPEATED -2
#define DIR_FILE_OCCUPIYED -3

struct orange_sb_info{
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
#define ORANGE_SB(sb) (&((sb)->orange_sb))
/**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	SUPER_BLOCK_SIZE	64		//modified by mingxuan 2020-10-30
#define	INVALID_INODE		0
#define	ROOT_INODE			1
#define	NR_ORANGE_INODE	64	/* FIXME */
/**
 * @struct inode
 * @brief  i-node
 *
 * The \c start_sect and\c nr_sects locate the file in the device,
 * and the size show how many bytes is used.
 * If <tt> size < (nr_sects * SECTOR_SIZE) </tt>, the rest bytes
 * are wasted and reserved for later writing.
 *
 * \b NOTE: Remember to change INODE_SIZE if the members are changed
 */
struct inode {
	u32	i_mode;		/**< Accsess mode */
	u32	i_size;		/**< File size */
	u32	i_start_block;	/**< The first block of the data */
	u32	i_nr_blocks;	/**< How many blocks the file occupies */
	u8	_unused[15];	/**< Stuff for alignment */
	u8 i_mnt_index; /**the index in mnt_table when the inode is mountpoint*/
	/* the following items are only present in memory */
	int	i_dev;
	int	i_cnt;		/**< How many procs share this inode  */
	int	i_num;		/**< inode nr.  */
};
extern struct superblock_operations orange_sb_ops;
extern struct inode_operations orange_inode_ops;
extern struct file_operations orange_file_ops;

struct orange_inode_info{
	u32	i_start_block;	/**< The first block of the data */
	u32	i_nr_blocks;	/**< How many blocks the file occupies */
};
/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	INODE_SIZE	32

/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
struct dir_entry {
	int	inode_nr;		/**< inode nr. */
	char name[MAX_FILENAME_LEN];	/**< Filename */
};

/**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */
#define	DIR_ENTRY_SIZE	sizeof(struct dir_entry)
/* //deleted by mingxuan 2019-5-17
PUBLIC int open(const char *pathname, int flags);
PUBLIC int close(int fd);
PUBLIC int read(int fd, void *buf, int count);
PUBLIC int write(int fd, const void *buf, int count);
PUBLIC int lseek(int fd, int offset, int whence);
PUBLIC int unlink(const char *pathname);
*/

//added by xw, 18/6/18
/* //deleted by mingxuan 2019-5-17
PUBLIC int sys_open(void *uesp);
PUBLIC int sys_close(void *uesp);
PUBLIC int sys_read(void *uesp);
PUBLIC int sys_write(void *uesp);
PUBLIC int sys_lseek(void *uesp);	//~xw
PUBLIC int sys_unlink(void *uesp);	//added by xw, 18/6/19
*/
// PUBLIC void init_rootfs(int device);
// PUBLIC int init_orangefs(int device);
// PUBLIC void create_mountpoint(const char *pathname, u32 dev,u8 index_mnt_table);
// PUBLIC void free_mountpoint(const char *pathname, u32 dev);
// PUBLIC int vfs_path_transfer(char* path,int* fs_index);

// //added by mingxuan 2019-5-17
// PUBLIC int real_open(struct super_block *sb,const char *pathname, int flags);
// PUBLIC int real_close(int fd);
// PUBLIC int real_read(int fd, char *buf, int count);
// PUBLIC int real_write(int fd, const char *buf, int count);
// PUBLIC int real_unlink(struct super_block *sb,const char *pathname);
// PUBLIC int real_lseek(int fd, int offset, int whence);

// PUBLIC int real_createdir(struct super_block *sb,const char *pathname); /*add by xkx 2023-1-3*/
// PUBLIC int real_opendir(struct super_block *sb,const char *pathname); /*add by xkx 2023-1-3*/
// PUBLIC int real_deletedir(struct super_block *sb,const char *pathname); /*add by xkx 2023-1-3*/
// PUBLIC int real_showdir(const char *pathname,char* dir_content);/*add by gfx 2023-2-13*/

// //added by mingxuan 2020-10-30
// PUBLIC void read_super_block(int dev);
// PUBLIC struct super_block* get_super_block(int dev);

// PUBLIC int get_blockfile_dev(char *path); //add by sundong 2023.5.28
// PUBLIC int create_tty_file(char *path,int tty_dev);//add by sundong 2023.5.18
// PUBLIC int create_blockdev_file(char *path, int block_dev_id);//add by sundong 2023.5.28
#endif /* FS_H */
