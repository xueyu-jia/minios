#include <minios/type.h>
#include <minios/vfs.h>
#define NR_MNT 10
#define MNT_DEVNAME_LEN 16
struct vfs_mount {
    char mnt_devname[MNT_DEVNAME_LEN];
    struct dentry* mnt_mountpoint;
    struct dentry* mnt_root;
    struct super_block* mnt_sb;
    int used;
};
// PUBLIC int get_fs_index(u8 index_mnt_table);
struct vfs_mount* lookup_vfsmnt(struct dentry* mountpoint);
struct vfs_mount* add_vfsmount(const char* dev_path, struct dentry* mnt_mountpoint,
                               struct dentry* mnt_root, struct super_block* sb);
struct dentry* remove_vfsmnt(struct dentry* entry);
