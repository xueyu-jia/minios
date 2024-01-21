#include "type.h"
#include "vfs.h"
#define MAX_mnt_table_length   10
//deleted by sundong 2023.5.19 vfs用于区分路径属于哪个文件系统，mountopen不再使用


struct vfs_mount{
	char dev_path[MAX_PATH];
	char dir_path[MAX_PATH];
	struct vfs_dentry * mnt_root;
    u32 dev;
	int used;
};
// PUBLIC int get_fs_index(u8 index_mnt_table);
PUBLIC struct vfs_mount* add_vfsmount(char* dev_path, char* dir, struct vfs_dentry* mnt_root, int dev);