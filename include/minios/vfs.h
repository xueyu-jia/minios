/**********************************************************
 *	vfs.h       //added by mingxuan 2019-5-17
 ***********************************************************/
#ifndef VFS_H
#define VFS_H
#include <minios/const.h>
#include <minios/fs.h>
#include <minios/stat.h>
// mark
// #define NR_DEV 10
// #define NR_FS 16		//modified by mingxuan 2020-10-18

// added by mingxuan 2020-10-27
#define NO_FS_TYPE 0x0 // added by mingxuan 2020-10-30
#define DEV_FS_TYPE 0x1
#define ORANGE_TYPE 0x2
#define FAT32_TYPE 0x3

// # define TTY_FS_TYPE	0x3	//added by mingxuan 2020-10-30

// #define DEV_NAME_LEN 128	//mark
// //#define NR_fs 3
// #define NR_FS_OP 3		//modified by mingxuan 2020-10-18
// #define NR_SB_OP 2		//added by mingxuan 2020-10-30

extern struct dentry *vfs_root;

struct inode *vfs_new_inode(struct super_block *sb);
struct inode *vfs_get_inode(struct super_block *sb, u32 ino);
void vfs_sync_inode(struct inode *inode);
struct dentry *vfs_new_dentry(const char *name, struct inode *inode);
int get_fstype_by_name(const char *fstype_name);

int sys_open();
int sys_close();
int sys_read();
int sys_write();
int sys_lseek();
int sys_unlink();
int sys_creat();
int sys_closedir();
int sys_opendir();
int sys_mkdir();
int sys_rmdir();
int sys_readdir();
int sys_chdir();  // added by ran
int sys_getcwd(); // added by ran
int sys_stat();

int kern_vfs_open(const char *path, int flags, int mode);
int kern_vfs_close(int fd);
int kern_vfs_read(int fd, char *buf, int count);
int kern_vfs_lseek(int fd, int offset, int whence);

void register_fs_types();
int kern_vfs_mount(const char *source, const char *target, const char *filesystemtype,
                   unsigned long mountflags, const void *data);
int kern_vfs_umount(const char *target);
void init_fs(int root_drive);
void vfs_put_dentry(struct dentry *dentry);
#endif
