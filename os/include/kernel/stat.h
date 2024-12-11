#ifndef STAT_H
#define STAT_H
#include <kernel/type.h>

struct stat {
    u32 st_dev;   /* ID of device containing file */
    u32 st_ino;   /* Inode number */
    int st_type;  /* File type */
    int st_mode;  /* File mode */
    u32 st_nlink; /* Number of hard links */
    u32 st_rdev;  /* Device ID (if special file) */
    u64 st_size;  /* Total size, in bytes */
    u32 st_atim;  /* Time of last access */
    u32 st_mtim;  /* Time of last modification */
    u32 st_crtim; /* Time of creation */
};

#endif
