#pragma once

enum {
    DEV_NONE,
    DEV_CHAR_TTY,
    DEV_CHAR_SERIAL,
    DEV_CHAR_RTC,
    DEV_BLK_FLOPPY,
    DEV_BLK_CDROM,
    DEV_BLK_HD_IDE,
    DEV_BLK_HD_SATA,
    DEV_BLK_HD_SCSI,
};

#define DEV_MAJOR_MASK 0xfff
#define DEV_MAJOR_SHIFT 20
#define DEV_MINOR_MASK 0xfffff
#define DEV_MINOR_SHIFT 0

#define DEV_MAKE_ID(major, minor) \
    ((((major) & DEV_MAJOR_MASK) << DEV_MAJOR_SHIFT) | (((minor) & DEV_MINOR_MASK) << DEV_MINOR_SHIFT))

#define DEV_MAJOR(x) (((x) >> DEV_MAJOR_SHIFT) & DEV_MAJOR_MASK)
#define DEV_MINOR(x) (((x) >> DEV_MINOR_SHIFT) & DEV_MINOR_MASK)
