#include "const.h"
#include "string.h"
#include "vfs.h"
#include "fs.h"
#include "mount.h"

struct mount_options{
	char target[MAX_PATH];
	int fs_type;
};
// PRIVATE void update_mnttable();
PUBLIC mount_table mnt_table[MAX_mnt_table_length];
extern struct vfs vfs_table[NR_FS];

PUBLIC int kern_mount(const char *source, const char *target,
                      const char *filesystemtype, unsigned long mountflags, const void *data);

PUBLIC int do_umount(const char *target);

// oprate
//PUBLIC int mount_open(char *pathname, int flags);

PRIVATE int alloc_mnttable();

PRIVATE void free_mnttable(char *target);


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

PRIVATE int alloc_mnttable()
{
    int i;
    for (i = 0; i < MAX_mnt_table_length; i++)
    {
        if (mnt_table[i].used == 0)
        {
            return i;
        }
    }
    return -1;
}

PRIVATE void free_mnttable(char *target)
{
    int i;
    for (i = 0; i < MAX_mnt_table_length; i++)
    {
        if (!strcmp(target, mnt_table[i].filename))
        {
            mnt_table[i].vfs_index = -1;
            memset(mnt_table[i].filename, 0, sizeof(mnt_table[i].filename));
            mnt_table[i].used = 0;
            mnt_table[i].dev = 0;
            return;
        }
    }
    disp_str("no such file! please check your target.");
    return;
}

PRIVATE int get_dev_from_name(char *devname)
{
    char ch = devname[6];
    char num = devname[7];
    int major = ch - 'a';

    if (devname[4] == 'h')
    {
        major += IDE_BASE;
    }
    else if (devname[4] == 's')
    {
        major += SATA_BASE;
    }

    int minor = num - '0';

    if (num == '\0')
        minor = 0;

    int dev_num = MAKE_DEV(major, minor);

    return dev_num;
}

PRIVATE int find_dev_in_mnttable(int device)
{
    for (int i = 0; i < MAX_mnt_table_length; i++)
    {
        if (mnt_table[i].dev == device)
        {
            return i;
        }
    }
    return -1;
}

PRIVATE int find_directory_in_mnttable(char *filename)
{
    for (int i = 0; i < MAX_mnt_table_length; i++)
    {
        if (strcmp(mnt_table[i].filename, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}
/**
* 根据mnt_table 中的下标索引来确定
* @param index_mnt_table 挂载点在mnt_table的下标
* @return -1代表查找失败 
*/
PUBLIC int get_fs_index(u8 index_mnt_table)
{
/*     for (int i = 0; i < MAX_mnt_table_length; i++)
    {
        if (!strcmp(mnt_table[i].filename, mountpoint_path))
        {
            return mnt_table[i].vfs_index;
        }
    } */
    if(index_mnt_table>=MAX_mnt_table_length)return -1;
    return mnt_table[index_mnt_table].vfs_index;
    
}
//deleted by sundong 2023.5.19 mount open 不再发挥作用，vfs中已经能够区分开路径属于哪个文件系统了
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
        fd = vfs_table[index].op->open(vfs_table[index].sb,mount_pathname, flags);
    }
    else // fat32
    {
        fd = vfs_table[index].op->open(vfs_table[index].sb,pathname, flags);
    }
    p_proc_current->task.filp[fd]->dev_index = index;

    return fd;
} */
PUBLIC void mount_root(){

}

PUBLIC int kern_mount(const char *source, const char *target,
                      const char *filesystemtype, unsigned long mountflags, const void *data)
{
    //将字符串从用户空间复制到内核空间
    char block_filepath[MAX_PATH];
    char mntpoint_path[MAX_PATH];
    memset(mntpoint_path,0,MAX_PATH);
    memset(block_filepath,0,MAX_PATH);
    strcpy(block_filepath,source);
    strcpy(mntpoint_path,target);

    // modified by sundong 2023.5.28
    //int device = get_dev_from_name(source);
    //从根文件系统中获取块设备的设备号
    int device = get_blockfile_dev(block_filepath);
    if (find_dev_in_mnttable(device) != -1)
    {
        disp_str("dev has already be mountted\n");
        return -1;
    }
    if (find_directory_in_mnttable(mntpoint_path) != -1)
    {
        disp_str("mountpoint has already be mountted\n");
        return -1;
    }

    int vfs_index;
    vfs_index = set_vfstable(device, mntpoint_path);

    if (vfs_index == -1)
        return -1;

    int mnt_index = alloc_mnttable();

    if (mnt_index >= 0)
    {
        mnt_table[mnt_index].vfs_index = vfs_index;
        strcpy(mnt_table[mnt_index].filename, mntpoint_path);
        mnt_table[mnt_index].used = 1;
        mnt_table[mnt_index].dev = device;
    }
    else
    {
        return -1;
    }

    create_mountpoint(mntpoint_path, vfs_table[3].sb->sb_dev,mnt_index);

    return 0;
}

PUBLIC int do_mount(const char *source, const char *target,
                    const char *filesystemtype, unsigned long mountflags, const void *data)
{
    return kern_mount(source, target, filesystemtype, mountflags, data);
}

PUBLIC int kern_umount(const char *target)
{
    free_mnttable(target);
    free_mountpoint(target, vfs_table[3].sb->sb_dev);
    // vfs_table[4].op ->unlink(target);
    return 0;
}

PUBLIC int do_umount(const char *target)
{
    kern_umount(target);
}

/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

PUBLIC int sys_mount()
{
    return do_mount(get_arg(1), get_arg(2), get_arg(3), get_arg(4), get_arg(5));
}

PUBLIC int sys_umount()
{
    return do_umount(get_arg(1));
}
