#ifndef DEV_H
#define DEV_H
#include <kernel/const.h>
#include <kernel/type.h>
#include <klib/list.h>

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define NO_DEV 0
#define DEV_FLOPPY 1
#define DEV_CDROM 2
#define DEV_HD_BASE 3
#define DEV_HD_LIMIT 11
// DEV_HD drive   hd dev = DEV_HD_BASE + drive
#define IDE_BASE 0
#define IDE_LIMIT 4
#define SATA_BASE 4
#define SATA_LIMIT 8
#define SCSI_BASE 8
#define SCSI_LIMIT 8

#define DEV_CHAR_TTY 13

/* make device number from major and minor numbers */
#define MAJOR_SHIFT 20
#define MAKE_DEV(a, b) (((a) << MAJOR_SHIFT) | (b))

/* separate major and minor numbers from device number */
#define MAJOR(x) ((x >> MAJOR_SHIFT) & 0x0FFF)
#define MINOR(x) (x & 0x0FFFFF)

#endif
