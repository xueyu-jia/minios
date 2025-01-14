#include <minios/mount.h>
#include <minios/vfs.h>
#include <minios/spinlock.h>
#include <fs/fs.h>
#include <string.h>

struct vfs_mount vfs_mnt_table[NR_MNT];
spinlock_t mnt_table_lock;

static struct vfs_mount* get_free_vfsmount() {
    int i;
    for (i = 0; i < NR_MNT; ++i) {
        if (vfs_mnt_table[i].used == 0) { return &vfs_mnt_table[i]; }
    }
    return NULL;
}

struct vfs_mount* add_vfsmount(const char* dev_path, struct dentry* mnt_mountpoint,
                               struct dentry* mnt_root, struct super_block* sb) {
    spinlock_acquire(&mnt_table_lock);
    struct vfs_mount* mnt = get_free_vfsmount();
    strcpy(mnt->mnt_devname, dev_path);
    mnt->mnt_sb = sb;
    mnt->mnt_mountpoint = mnt_mountpoint;
    mnt->mnt_root = mnt_root;
    mnt->used = 1;
    spinlock_release(&mnt_table_lock);
    return mnt;
}

struct vfs_mount* lookup_vfsmnt(struct dentry* mountpoint) {
    struct vfs_mount* mnt = vfs_mnt_table;
    spinlock_acquire(&mnt_table_lock);
    while (mnt < vfs_mnt_table + NR_MNT) {
        if (mnt->mnt_mountpoint == mountpoint && mnt->used == 1) { break; }
        mnt++;
    }
    spinlock_release(&mnt_table_lock);
    return mnt;
}

struct dentry* remove_vfsmnt(struct dentry* entry) {
    struct vfs_mount* mnt = vfs_mnt_table;
    struct dentry* mountpoint = NULL;
    spinlock_acquire(&mnt_table_lock);
    while (mnt < vfs_mnt_table + NR_MNT) {
        if (mnt->mnt_root == entry && mnt->used == 1) {
            mnt->used = 0;
            mountpoint = mnt->mnt_mountpoint;
            break;
        }
        mnt++;
    }
    mnt->mnt_sb->sb_vfsmount = NULL;
    spinlock_release(&mnt_table_lock);
    return mountpoint;
}
