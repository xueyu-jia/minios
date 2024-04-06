#include "type.h"
#include "vfs.h"
#define NR_MNT   10

struct vfs_mount{
	char mnt_devname[12];
	struct vfs_dentry * mnt_mountpoint;
	struct vfs_dentry * mnt_root;
    struct super_block* mnt_sb;
	int used;
};
// PUBLIC int get_fs_index(u8 index_mnt_table);
PUBLIC struct vfs_mount* lookup_vfsmnt(struct vfs_dentry* mountpoint);
PUBLIC struct vfs_mount* add_vfsmount(const char *dev_path, 
	struct vfs_dentry * mnt_mountpoint, struct vfs_dentry* mnt_root, struct super_block* sb);