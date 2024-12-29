#include <minios/const.h>
#include <minios/fs.h>
#include <minios/mount.h>
#include <minios/proto.h>
#include <minios/vfs.h>
#include <string.h>

// PRIVATE void update_mnttable();
// PUBLIC mount_table mnt_table[MAX_mnt_table_length];
struct vfs_mount vfs_mnt_table[NR_MNT];
SPIN_LOCK mnt_table_lock;
// extern struct vfs vfs_table[NR_FS];

// PUBLIC int kern_mount(const char *source, const char *target,
//                       const char *filesystemtype, unsigned long mountflags,
//                       const void *data);

int do_umount(const char* target);

// oprate
// PUBLIC int mount_open(char *pathname, int flags);

// PRIVATE int alloc_mnttable();

// PRIVATE void free_mnttable(char *target);

// PRIVATE void update_mnttable()
// {
//     int i;
//     for(i = 0; i < MAX_mnt_table_length; i++)
//     {
//         int inode_nr = search_file(mnt_table[i].filename);
//         int orange_dev = get_fs_dev(PRIMARY_MASTER, ORANGE_TYPE);
//         struct inode* mnt_node = get_inode(orange_dev, inode_nr);
//         mnt_node -> i_mode = I_MOUNTPOINT;
//         // put_inode(mnt_node);
//         // mnt_node -> i_cnt--;
//     }
// }

// PRIVATE int alloc_mnttable()
// {
//     int i;
//     for (i = 0; i < MAX_mnt_table_length; i++)
//     {
//         if (mnt_table[i].used == 0)
//         {
//             return i;
//         }
//     }
//     return -1;
// }

// PRIVATE void free_mnttable(char *target)
// {
//     int i;
//     for (i = 0; i < MAX_mnt_table_length; i++)
//     {
//         if (!strcmp(target, mnt_table[i].filename))
//         {
//             mnt_table[i].vfs_index = -1;
//             memset(mnt_table[i].filename, 0, sizeof(mnt_table[i].filename));
//             mnt_table[i].used = 0;
//             mnt_table[i].dev = 0;
//             return;
//         }
//     }
//     kprintf("no such file! please check your target.");
//     return;
// }

// PRIVATE int get_dev_from_name(char *devname)
// {
//     char ch = devname[6];
//     char num = devname[7];
//     int major = ch - 'a';

// if (devname[4] == 'h')
// {
//     major += IDE_BASE;
// }
// else if (devname[4] == 's')
// {
//     major += SATA_BASE;
// }

// int minor = num - '0';

// if (num == '\0')
//     minor = 0;

// int dev_num = MAKE_DEV(major, minor);

// return dev_num;
// }

// PRIVATE int find_dev_in_mnttable(int device)
// {
//     for (int i = 0; i < MAX_mnt_table_length; i++)
//     {
//         if (mnt_table[i].dev == device)
//         {
//             return i;
//         }
//     }
//     return -1;
// }

// PRIVATE int find_directory_in_mnttable(char *filename)
// {
//     for (int i = 0; i < MAX_mnt_table_length; i++)
//     {
//         if (strcmp(mnt_table[i].filename, filename) == 0)
//         {
//             return i;
//         }
//     }
//     return -1;
// }
// /**
// * 根据mnt_table 中的下标索引来确定
// * @param index_mnt_table 挂载点在mnt_table的下标
// * @return -1代表查找失败
// */
// PUBLIC int get_fs_index(u8 index_mnt_table)
// {
// /*     for (int i = 0; i < MAX_mnt_table_length; i++)
//     {
//         if (!strcmp(mnt_table[i].filename, mountpoint_path))
//         {
//             return mnt_table[i].vfs_index;
//         }
//     } */
//     if(index_mnt_table>=MAX_mnt_table_length)return -1;
//     return mnt_table[index_mnt_table].vfs_index;

// }
// deleted by sundong 2023.5.19 mount open
// 不再发挥作用，vfs中已经能够区分开路径属于哪个文件系统了
/* PUBLIC int mount_open(char *pathname, int flags)
{
    char orange_pathname[20];
    char mount_pathname[20];

    int i, j;
    for (i = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            j = i;
            break;
        }
    }



    for (i = 0; i < j; i++)
    {
        orange_pathname[i] = pathname[i];
    }
    orange_pathname[i] = 0;

    for (i = j + 1; i < strlen(pathname); i++)
    {
        mount_pathname[i - j - 1] = pathname[i];
    }
    mount_pathname[i - j - 1] = 0;

    int index;
    for (i = 0; i < MAX_mnt_table_length; i++)
    {
        if (!strcmp(mnt_table[i].filename, orange_pathname))
        {
            index = mnt_table[i].vfs_index;
            break;
        }
    }

    int fd;
    if (vfs_table[index].op->open == real_open) // orange
    {
        fd = vfs_table[index].op->open(vfs_table[index].sb,mount_pathname,
flags);
    }
    else // fat32
    {
        fd = vfs_table[index].op->open(vfs_table[index].sb,pathname, flags);
    }
    p_proc_current->task.filp[fd]->dev_index = index;

    return fd;
} */

static struct vfs_mount* get_free_vfsmount() {
    int i;
    for (i = 0; i < NR_MNT; i++) {
        if (vfs_mnt_table[i].used == 0) { return &vfs_mnt_table[i]; }
    }
    return NULL;
}

struct vfs_mount* add_vfsmount(const char* dev_path, struct dentry* mnt_mountpoint,
                               struct dentry* mnt_root, struct super_block* sb) {
    acquire(&mnt_table_lock);
    struct vfs_mount* mnt = get_free_vfsmount();
    strcpy(mnt->mnt_devname, dev_path);
    mnt->mnt_sb = sb;
    mnt->mnt_mountpoint = mnt_mountpoint;
    mnt->mnt_root = mnt_root;
    mnt->used = 1;
    release(&mnt_table_lock);
    return mnt;
}

struct vfs_mount* lookup_vfsmnt(struct dentry* mountpoint) {
    struct vfs_mount* mnt = vfs_mnt_table;
    acquire(&mnt_table_lock);
    while (mnt < vfs_mnt_table + NR_MNT) {
        if (mnt->mnt_mountpoint == mountpoint && mnt->used == 1) { break; }
        mnt++;
    }
    release(&mnt_table_lock);
    return mnt;
}

struct dentry* remove_vfsmnt(struct dentry* entry) {
    struct vfs_mount* mnt = vfs_mnt_table;
    struct dentry* mountpoint = NULL;
    acquire(&mnt_table_lock);
    while (mnt < vfs_mnt_table + NR_MNT) {
        if (mnt->mnt_root == entry && mnt->used == 1) {
            mnt->used = 0;
            mountpoint = mnt->mnt_mountpoint;
            break;
        }
        mnt++;
    }
    mnt->mnt_sb->sb_vfsmount = NULL;
    release(&mnt_table_lock);
    return mountpoint;
}

int do_mount(const char* source, const char* target, const char* filesystemtype,
             unsigned long mountflags, const void* data) {
    return kern_vfs_mount(source, target, filesystemtype, mountflags, data);
}

// PUBLIC int kern_umount(const char *target)
// {
//     free_mnttable(target);
//     free_mountpoint(target, vfs_table[3].sb->sb_dev);
//     // vfs_table[4].op ->unlink(target);
//     return 0;
// }

int do_umount(const char* target) {
    // kern_umount(target);
    return kern_vfs_umount(target);
}

/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

int sys_mount() {
    return do_mount((const char*)get_arg(1), (const char*)get_arg(2), (const char*)get_arg(3),
                    get_arg(4), (const void*)get_arg(5));
}

int sys_umount() {
    return do_umount((const char*)get_arg(1));
}
