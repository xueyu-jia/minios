#ifndef DEV_H
#define DEV_H
#include "type.h"
#include "const.h"
#include "list.h"
#include "fs.h"

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define NO_DEV       0
#define DEV_FLOPPY   1
#define DEV_CDROM    2
#define DEV_HD_BASE  3
#define DEV_HD_LIMIT 11
// DEV_HD drive   hd dev = DEV_HD_BASE + drive
#define IDE_BASE   0
#define IDE_LIMIT  4
#define SATA_BASE  4
#define SATA_LIMIT 8
#define SCSI_BASE  8
#define SCSI_LIMIT 8

#define DEV_CHAR_TTY 13

#define DEV_CHAR_TYPE  1
#define DEV_BLOCK_TYPE 2

/* make device number from major and minor numbers */
#define MAJOR_SHIFT    20
#define MAKE_DEV(a, b) (((a) << MAJOR_SHIFT) | (b))

/* separate major and minor numbers from device number */
#define MAJOR(x) ((x >> MAJOR_SHIFT) & 0x0FFF)
#define MINOR(x) (x & 0x0FFFFF)

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
PUBLIC struct device        *
    register_device(int dev, int type, struct file_operations *fop);
PUBLIC int unregister_device(int dev);

#endif