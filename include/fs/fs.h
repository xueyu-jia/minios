#pragma once

#include <uapi/minios/fs.h>
#include <fs/fs_const.h>
#include <minios/spinlock.h>
#include <minios/cache.h>
#include <minios/mempage.h>
#include <klib/stdint.h>
#include <list.h>

struct inode;
struct dentry;
struct file_desc;
struct super_block;

struct fs_size_info {
    int sb_size;
    int inode_size;
};

struct inode {
    u32 i_no;
    struct super_block *i_sb;
    u32 i_dev;               // real device
    u32 i_b_cdev;            // for block/char special inode
    u32 i_nlink;             // file linked
    atomic_t i_count;        // reference count
    u64 i_size;              // file size in byte
    int i_type;              // char/blk/mnt/dir...
    int i_mode;              // permission ==> I_R/W/X
    u32 i_atime;             // access time (use UTC timestamp, 下同)
    u32 i_crtime;            // create time
    u32 i_mtime;             // modify time
    list_head i_dentry;      // connected dentry list，待定
    struct list_node i_list; // recently used inodes list
    // contains inode cached pages, ordered by page offset
    address_space_t *i_mapping;
    address_space_t i_data;
    struct inode_operations *i_op;
    struct file_operations *i_fop;
    spinlock_t lock;
    //! NOTE: private data maintained by fs instance
    //! ATTENTION: struct inode must be allocated dynamically due to unknown
    //! private data size
    char i_private[0];
};

// **** dentry 的本质是cache !!! ****
struct dentry {
    atomic_t d_count;
    // 对于一般的文件，直接使用d_shortname即可，过长的文件名另单独分配内存
    char d_shortname[MAX_DNAME_LEN];
    char *d_longname;
    struct inode *d_inode;
    struct list_node d_alias;
    struct list_node d_list;
    list_head d_subdirs;
    struct dentry *d_parent;
    struct vfs_mount *d_vfsmount;
    int d_mounted;
    struct dentry_operations *d_op;
    spinlock_t lock;
};
#define dentry_name(d) ((d)->d_longname ? (d)->d_longname : (d)->d_shortname)
/**
 * @struct super_block fs.h "include/fs.h"
 *
 */
// modified by sundong 2023.5.26
struct super_block {
    struct dentry *sb_root;        // 根dentry
    struct vfs_mount *sb_vfsmount; //
    list_head sb_inode_list;
    struct superblock_operations *sb_op;
    int sb_blocksize;
    int sb_dev;  /**< the super block's home device */
    int fs_type; // added by mingxuan 2020-10-30
    int used;
    spinlock_t lock;
    //! NOTE: private data maintained by fs instance
    //! ATTENTION: struct super_block must be allocated dynamically due to
    //! unknown private data size
    char sb_private[0];
};

struct file_desc {
    int fd_mode;       /**< R or W */
    atomic_t fd_count; // 用于维护进程间相同File引用的资源释放
    u64 fd_pos;        /**< Current position for R/W. */
    struct dentry *fd_dentry;
    address_space_t *fd_mapping;
    struct file_operations *fd_ops;
};

#define fget(file) atomic_inc(&((file)->fd_count))
void fput(struct file_desc *file);

struct dirent {
    int d_len; // total len, including full d_name
    int d_ino;
    char d_name[1]; // just d_name start
};
#define dirent_next(dent) ((struct dirent *)(((char *)dent) + (dent->d_len)))
#define dirent_len(name_len) ((u32)(name_len) + 9) // 2*int+end'\0'
#define dirent_fill(dent, ino, name_len)      \
    do {                                      \
        (dent)->d_ino = ino;                  \
        (dent)->d_len = dirent_len(name_len); \
    } while (0)
// opendir mallock a page and take beginning as dirstream struct, remaining data
// as dir raw dirent | ------4K-------------| |dirstream|dirent... |
struct dirstream {
    struct file_desc *file;
    int init;
    int pos;
    int total;
};
#define DIR_DATA(dirp) ((struct dirent *)((dirp) + 1))

typedef struct dirstream DIR;

struct inode_operations {
    struct dentry *(*lookup)(struct inode *dir, const char *filename);
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, int mode);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
    int (*get_block)(struct inode *inode, u32 iblock, int create);
};

struct dentry_operations {
    int (*compare)(const char *dentryname, const char *filename);
};

struct file_operations {
    int (*read)(struct file_desc *file, unsigned int count, char *buf);
    int (*write)(struct file_desc *file, unsigned int count, const char *buf);
    int (*readdir)(struct file_desc *file, unsigned int count, struct dirent *start);
    int (*fsync)(struct file_desc *file, int datasync);
};

struct superblock_operations {
    void (*query_size_info)(struct fs_size_info *);
    int (*fill_superblock)(struct super_block *, int);
    int (*sync_inode)(struct inode *inode);
    void (*read_inode)(struct inode *inode);
    void (*put_inode)(struct inode *inode);
    void (*delete_inode)(struct inode *inode);
};

struct fs_type {
    char *fstype_name;
    struct fs_size_info fs_size_info;
    struct file_operations *fs_fop;
    struct inode_operations *fs_iop;
    struct superblock_operations *fs_sop;
    struct dentry_operations *fs_dop;
    int (*identify)(int drive, u32 start_sector);
};

extern struct super_block *super_blocks[NR_SUPER_BLOCK];
extern struct fs_type fstype_table[NR_FS_TYPE];

int get_free_superblock();
void generic_query_size_info(struct fs_size_info *info);
int generic_file_readpage(address_space_t *file_mapping, memory_page_t *target);
int generic_file_writepage(address_space_t *file_mapping, memory_page_t *target);
int generic_file_read(struct file_desc *file, unsigned int count, char *buf);
int generic_file_write(struct file_desc *file, unsigned int count, const char *buf);
int generic_file_fsync(struct file_desc *file, int datasync);
