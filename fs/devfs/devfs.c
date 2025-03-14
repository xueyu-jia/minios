#include <fs/devfs/devfs.h>
#include <minios/hd.h>
#include <minios/clock.h>
#include <minios/vfs.h>
#include <minios/console.h>
#include <minios/memman.h>
#include <minios/spinlock.h>
#include <minios/assert.h>
#include <minios/dev.h>
#include <klib/size.h>
#include <string.h>
#include <fmt.h>

static list_head devices;
static spinlock_t devices_lock;

void init_devices() {
    list_init(&devices);
    spinlock_init(&devices_lock, NULL);
}

int devfs_attach_device(device_set_t *devset, int dev) {
    if (devset == NULL) { return -1; }
    if (DEV_MAJOR(dev) != devset->type) { return -1; }
    spinlock_acquire(&devices_lock);
    device_t *device = NULL;
    list_for_each(&devset->devices, device, self) {
        if (device->id == dev) { break; }
    }
    const bool already_attached = device != NULL;
    if (!already_attached) {
        device = kern_kmalloc(sizeof(device_t));
        assert(device != NULL);
        list_init(&device->self);
        device->id = dev;
        list_add_last(&device->self, &devset->devices);
    }
    spinlock_release(&devices_lock);
    return already_attached ? -1 : 0;
}

int devfs_detach_device_unsafe(device_set_t *devset, int dev) {
    if (devset == NULL) { return -1; }
    if (DEV_MAJOR(dev) != devset->type) { return -1; }
    spinlock_acquire(&devices_lock);
    device_t *device = NULL;
    list_for_each(&devset->devices, device, self) {
        if (device->id == dev) { break; }
    }
    if (device != NULL) {
        list_remove(&device->self);
        kern_kfree(device);
    }
    spinlock_release(&devices_lock);
    return device != NULL ? 0 : -1;
}

device_set_t *devfs_find_devset(int dev) {
    const int type = DEV_MAJOR(dev);
    spinlock_acquire(&devices_lock);
    device_set_t *devset = NULL;
    list_for_each(&devices, devset, self) {
        if (devset->type == type) { break; }
    }
    spinlock_release(&devices_lock);
    return devset;
}

device_t *devfs_find_device(device_set_t *devset, int dev) {
    if (devset == NULL) { return NULL; }
    if (DEV_MAJOR(dev) != devset->type) { return NULL; }
    spinlock_acquire(&devices_lock);
    device_t *device = NULL;
    list_for_each(&devset->devices, device, self) {
        if (device->id == dev) { break; }
    }
    spinlock_release(&devices_lock);
    return device;
}

device_set_t *devfs_find_or_alloc_devset(int dev, struct file_operations *fops) {
    //! FIXME: we want a recursive lock here
    device_set_t *devset = devfs_find_devset(dev);
    if (devset != NULL) { return devset; }
    if (fops == NULL) { return NULL; }
    devset = kern_kmalloc(sizeof(device_set_t));
    assert(devset != NULL);
    list_init(&devset->self);
    list_init(&devset->devices);
    devset->file_ops = fops;
    devset->type = DEV_MAJOR(dev);
    spinlock_acquire(&devices_lock);
    list_add_last(&devset->self, &devices);
    spinlock_release(&devices_lock);
    return devset;
}

int devfs_release_devset_unsafe(int dev) {
    //! FIXME: we want a recursive lock here
    device_set_t *devset = devfs_find_devset(dev);
    if (devset == NULL) { return -1; }
    spinlock_acquire(&devices_lock);
    list_remove(&devset->self);
    spinlock_release(&devices_lock);
    device_t *device = NULL;
    while ((device = list_front(&devset->devices, device_t, self)) != NULL) {
        list_remove(&device->self);
        kern_kfree(device);
    }
    return 0;
}

int register_device(int dev, struct file_operations *fops) {
    device_set_t *devset = devfs_find_or_alloc_devset(dev, fops);
    if (devset == NULL) { return -1; }
    return devfs_attach_device(devset, dev);
}

int unregister_device(int dev) {
    device_set_t *devset = devfs_find_devset(dev);
    if (devset == NULL) { return -1; }
    const int retval = devfs_detach_device_unsafe(devset, dev);
    if (retval != 0) { return -1; }
    return list_empty(&devset->devices) ? devfs_release_devset_unsafe(dev) : 0;
}

int devfs_read_dev_name(int dev, void *buf, int n) {
    switch (DEV_MAJOR(dev)) {
        case DEV_CHAR_TTY: {
            nstrfmt(buf, n, "tty%d", DEV_MINOR(dev));
        } break;
        case DEV_CHAR_SERIAL: {
            nstrfmt(buf, n, "ttyS%d", DEV_MINOR(dev));
        } break;
        case DEV_CHAR_RTC: {
            nstrfmt(buf, n, "rtc%d", DEV_MINOR(dev));
        } break;
        case DEV_BLK_HD_SATA: {
            if (HD_DEV_IS_BLK(dev)) {
                nstrfmt(buf, n, "sd%c", 'a' + HD_DEV_GET_INDEX(dev));
            } else {
                nstrfmt(buf, n, "sd%c%d", 'a' + HD_DEV_GET_INDEX(dev), HD_DEV_GET_PART(dev) + 1);
            }
        } break;
        default: {
            return -1;
        } break;
    }
    return 0;
}

void devfs_read_inode(struct inode *inode) {
    inode->i_crtime = current_timestamp;
    inode->i_atime = current_timestamp;
    inode->i_mtime = current_timestamp;
    inode->i_nlink = 1;

    if (inode->i_no == 1) {
        inode->i_mode = I_R | I_X;
        inode->i_type = I_DIRECTORY;
        inode->i_size = SZ_4K;
        inode->i_op = &devfs_inode_ops;
        inode->i_fop = &devfs_file_ops;
        return;
    }

    const int dev = inode->i_no;
    device_set_t *devset = devfs_find_devset(dev);
    assert(devset != NULL);
    assert(devfs_find_device(devset, dev) != NULL);

    inode->i_mode = I_R | I_W;
    inode->i_b_cdev = dev;
    inode->i_fop = devset->file_ops;

    switch (devset->type) {
        case DEV_BLK_FLOPPY:
            FALLTHROUGH;
        case DEV_BLK_CDROM:
            FALLTHROUGH;
        case DEV_BLK_HD_IDE:
            FALLTHROUGH;
        case DEV_BLK_HD_SATA:
            FALLTHROUGH;
        case DEV_BLK_HD_SCSI: {
            const hd_info_t *drv = hd_lookup(dev);
            assert(drv != NULL);
            const uint64_t nr_sectors =
                HD_DEV_IS_BLK(dev) ? drv->parts[0].size : drv->parts[HD_DEV_GET_PART(dev) + 1].size;
            inode->i_type = I_BLOCK_SPECIAL;
            inode->i_size = nr_sectors * SECTOR_SIZE;
        } break;
        case DEV_CHAR_TTY:
            FALLTHROUGH;
        case DEV_CHAR_SERIAL:
            FALLTHROUGH;
        case DEV_CHAR_RTC: {
            inode->i_type = I_CHAR_SPECIAL;
            inode->i_size = 0;
        } break;
        default: {
            unreachable();
        } break;
    }
}

struct dentry *devfs_lookup(struct inode *dir, const char *filename) {
    char name[8] = {};
    device_set_t *devset = NULL;
    device_t *device = NULL;
    bool found = false;

    spinlock_acquire(&devices_lock);
    list_for_each(&devices, devset, self) {
        list_for_each(&devset->devices, device, self) {
            const int retval = devfs_read_dev_name(device->id, name, sizeof(name));
            assert(retval == 0);
            if (strcmp(name, filename) != 0) { continue; }
            found = true;
            break;
        }
        if (found) { break; }
    }
    spinlock_release(&devices_lock);

    if (!found) { return NULL; }

    struct inode *inode = vfs_get_inode(dir->i_sb, device->id);
    return vfs_new_dentry(name, inode);
}

int devfs_readdir(struct file_desc *file, unsigned int count, struct dirent *start) {
    UNUSED(file, count);

    struct dirent *dent = start;

    dirent_fill(dent, 1, 1);
    strcpy(dent->d_name, ".");
    count -= dent->d_len;
    dent = dirent_next(dent);

    dirent_fill(dent, 1, 2);
    strcpy(dent->d_name, "..");
    count -= dent->d_len;
    dent = dirent_next(dent);

    char name[8] = {};
    device_set_t *devset = NULL;
    device_t *device = NULL;

    spinlock_acquire(&devices_lock);
    list_for_each(&devices, devset, self) {
        list_for_each(&devset->devices, device, self) {
            const int retval = devfs_read_dev_name(device->id, name, sizeof(name));
            assert(retval == 0);
            dirent_fill(dent, device->id, strlen(name));
            strcpy(dent->d_name, name);
            count -= dent->d_len;
            dent = dirent_next(dent);
        }
    }
    spinlock_release(&devices_lock);

    return ptr2u(dent) - ptr2u(start);
}

int devfs_fill_superblock(struct super_block *sb, int dev) {
    sb->sb_dev = dev;
    sb->sb_op = &devfs_sb_ops;
    sb->fs_type = FS_TYPE_DEV;
    struct inode *root = vfs_get_inode(sb, 1);
    sb->sb_root = vfs_new_dentry("dev", root);
    return 0;
}

struct superblock_operations devfs_sb_ops = {
    .query_size_info = generic_query_size_info,
    .fill_superblock = devfs_fill_superblock,
    .sync_inode = NULL,
    .read_inode = devfs_read_inode,
    .put_inode = NULL,
    .delete_inode = NULL,
};

struct inode_operations devfs_inode_ops = {
    .lookup = devfs_lookup,
    .create = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .get_block = NULL,
};

struct file_operations devfs_file_ops = {
    .read = NULL,
    .write = NULL,
    .readdir = devfs_readdir,
    .fsync = NULL,
};
