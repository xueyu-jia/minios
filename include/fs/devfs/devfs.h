#pragma once

#include <fs/fs.h>

typedef struct {
    struct list_node self;
    list_head devices;
    struct file_operations *file_ops;
    int type;
} device_set_t;

typedef struct {
    struct list_node self;
    int id;
} device_t;

extern struct superblock_operations devfs_sb_ops;
extern struct inode_operations devfs_inode_ops;
extern struct file_operations devfs_file_ops;

void init_devices();

int register_device(int dev, struct file_operations *fops);
int unregister_device(int dev);
