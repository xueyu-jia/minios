#pragma once

#include <stdint.h>

struct stat {
    uint32_t st_dev;   /* ID of device containing file */
    uint32_t st_ino;   /* Inode number */
    int st_type;       /* File type */
    int st_mode;       /* File mode */
    uint32_t st_nlink; /* Number of hard links */
    uint32_t st_rdev;  /* Device ID (if special file) */
    uint64_t st_size;  /* Total size, in bytes */
    uint32_t st_atim;  /* Time of last access */
    uint32_t st_mtim;  /* Time of last modification */
    uint32_t st_crtim; /* Time of creation */
};
