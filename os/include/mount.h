#include "type.h"
#include "vfs.h"
#define MAX_mnt_table_length   10
//deleted by sundong 2023.5.19 vfs用于区分路径属于哪个文件系统，mountopen不再使用
//extern int mount_open(char *pathname, int flags);
// extern void update_mnttable();
typedef struct
{
    char filename[16];
    u32 vfs_index;
    u32 dev;
    u32 used;//free=0
    /* data */
}mount_table;
struct vfs_mount{
	char dev_path[MAX_PATH];
	char dir_path[MAX_PATH];
	struct vfs_dentry * mnt_root;
    u32 dev;
	int used;
};
PUBLIC int get_fs_index(u8 index_mnt_table);
PUBLIC struct vfs_mount* add_vfsmount(char* dev_path, char* dir, struct vfs_dentry* mnt_root, int dev);