#ifndef DEVFS_H
#define DEVFS_H
#include <kernel/dev.h>
#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/fs.h>

#define DEV_CHAR_TYPE  1
#define DEV_BLOCK_TYPE 2

// 每个device结构体对应一个主设备号的设备
struct device {
    int                     dev_major;
    int                     dev_type; // CHAR / BLOCK
    u32                     dev_minor_map;
    struct file_operations *dev_fop;
    struct list_node        dev_list;
};

extern struct superblock_operations devfs_sb_ops;
extern struct inode_operations      devfs_inode_ops;
extern struct file_operations       devfs_file_ops;

PUBLIC void init_devices();
PUBLIC struct device *register_device(int dev, int type, struct file_operations *fop);
PUBLIC int unregister_device(int dev);

#endif
