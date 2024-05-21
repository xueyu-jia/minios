/**********************************************************
*	vfs.h       //added by mingxuan 2019-5-17
***********************************************************/
#ifndef VFS_H
#define VFS_H
#include "const.h"
#include "fs.h"
#include "stat.h"
// mark
//#define NR_DEV 10
// #define NR_FS 16		//modified by mingxuan 2020-10-18

// added by mingxuan 2020-10-27
# define NO_FS_TYPE		0x0	//added by mingxuan 2020-10-30
# define DEV_FS_TYPE	0x1
# define ORANGE_TYPE 	0x2
# define FAT32_TYPE	 	0x3

// # define TTY_FS_TYPE	0x3	//added by mingxuan 2020-10-30

// #define DEV_NAME_LEN 128	//mark
// //#define NR_fs 3
// #define NR_FS_OP 3		//modified by mingxuan 2020-10-18
// #define NR_SB_OP 2		//added by mingxuan 2020-10-30

extern struct dentry *vfs_root;

PUBLIC struct inode * vfs_new_inode(struct super_block* sb);
PUBLIC struct inode * vfs_get_inode(struct super_block* sb, int ino);
PUBLIC void vfs_sync_inode(struct inode * inode);
PUBLIC struct dentry * vfs_new_dentry(const char* name, struct inode* inode);



PUBLIC int sys_open();
PUBLIC int sys_close();
PUBLIC int sys_read();
PUBLIC int sys_write();
PUBLIC int sys_lseek();
PUBLIC int sys_unlink();
PUBLIC int sys_creat();
PUBLIC int sys_closedir();
PUBLIC int sys_opendir();
PUBLIC int sys_mkdir();
PUBLIC int sys_rmdir();
PUBLIC int sys_readdir();
PUBLIC int sys_chdir(); //added by ran
PUBLIC int sys_getcwd(); //added by ran
PUBLIC int sys_stat();

PUBLIC int kern_vfs_open(const char *path, int flags, int mode);
PUBLIC int kern_vfs_close(int fd);
PUBLIC int kern_vfs_read(int fd, char *buf, int count);
PUBLIC int kern_vfs_lseek(int fd, int offset, int whence);

PUBLIC void register_fs_types();
PUBLIC int kern_vfs_mount(const char *source, const char *target,
                      const char *filesystemtype, unsigned long mountflags, const void *data);
PUBLIC int kern_vfs_umount(const char *target);
PUBLIC void init_fs(int root_drive);
#endif