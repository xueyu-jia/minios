#pragma once

#include <fs/fs.h>

enum {
    FS_TYPE_NONE,
    FS_TYPE_DEV,
    FS_TYPE_ORANGE,
    FS_TYPE_FAT32,
    FS_TYPE_EXT4,
};

struct stat;

extern struct dentry *vfs_root;

void init_fs(int root_drive);

void register_fs_types();
int get_fstype_by_name(const char *fstype_name);

struct inode *vfs_new_inode(struct super_block *sb);
struct inode *vfs_get_inode(struct super_block *sb, u32 ino);
void vfs_sync_inode(struct inode *inode);
struct dentry *vfs_new_dentry(const char *name, struct inode *inode);
void vfs_put_dentry(struct dentry *dentry);

char *kern_vfs_getcwd(char *buf, int size);
DIR *kern_vfs_opendir(const char *path);
int kern_vfs_chdir(const char *path);
int kern_vfs_close(int fd);
int kern_vfs_closedir(DIR *dirp);
int kern_vfs_lseek(int fd, int offset, int whence);
int kern_vfs_mkdir(const char *path, int mode);
int kern_vfs_mount(const char *source, const char *target, const char *filesystemtype,
                   int mountflags, const void *data);
int kern_vfs_open(const char *path, int flags, int mode);
int kern_vfs_read(int fd, char *buf, int count);
int kern_vfs_rmdir(const char *path);
int kern_vfs_stat(const char *path, struct stat *statbuf);
int kern_vfs_umount(const char *target);
int kern_vfs_unlink(const char *path);
int kern_vfs_write(int fd, const char *buf, int count);
struct dirent *kern_vfs_readdir(DIR *dirp);
