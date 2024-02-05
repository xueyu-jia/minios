#include "type.h"
#include "vfs.h"
#define MAX_mnt_table_length   10
//deleted by sundong 2023.5.19 vfs用于区分路径属于哪个文件系统，mountopen不再使用


struct vfs_mount{
	struct vfs_dentry * mnt_dev;
	struct vfs_dentry * mnt_mountpoint;
	struct vfs_dentry * mnt_root;
    struct super_block* mnt_sb;
	int used;
};
// PUBLIC int get_fs_index(u8 index_mnt_table);
PUBLIC struct vfs_mount* add_vfsmount(struct vfs_dentry* dev_path, 
	struct vfs_dentry * mnt_mountpoint, struct vfs_dentry* mnt_root, struct super_block* sb);