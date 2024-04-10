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

extern struct vfs_dentry *vfs_root;

PUBLIC struct vfs_inode * vfs_new_inode(struct super_block* sb);
PUBLIC struct vfs_inode * vfs_get_inode(struct super_block* sb, int ino);
PUBLIC struct vfs_dentry * vfs_new_dentry(const char* name, struct vfs_inode* inode);



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

PUBLIC int do_vopen(const char *path, int flags, int mode);
PUBLIC int do_vclose(int fd);
PUBLIC int do_vread(int fd, char *buf, int count);
PUBLIC int do_vwrite(int fd, const char *buf, int count);
PUBLIC int do_vunlink(const char *path);
PUBLIC int do_vlseek(int fd, int offset, int whence);
PUBLIC int do_vcreat(const char *pathname);
PUBLIC int do_vclosedir(DIR* dirp);
PUBLIC int do_vopendir(const char *dirname);
PUBLIC int do_vmkdir(const char *dirname, int mode);
PUBLIC int do_vrmdir(const char *dirname);
PUBLIC int do_vchdir(const char *path); //added by ran
//PUBLIC char* do_vgetcwd(char *buf, int size); //added by ran
PUBLIC int do_vgetcwd(char *buf, int size); //modified by mingxuan 2021-8-15
PUBLIC int do_vstat(const char *pathname, struct stat* statbuf);
// PUBLIC int set_vfstable(u32 device, char *target);
// PUBLIC struct vfs* vfs_alloc_vfs_entity();
// PUBLIC int get_index(char path[]);
// PUBLIC void init_vfs();
PUBLIC void register_fs_types();
PUBLIC void init_fs();
#endif