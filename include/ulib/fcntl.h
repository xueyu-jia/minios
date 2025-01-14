#pragma once

#include <sys/types.h>
#include <uapi/minios/fs.h> // IWYU pragma: export

struct dirent {
    int d_len; // total len, including full d_name
    int d_ino;
    char d_name[MAX_PATH]; // just d_name start
};

typedef struct dirstream DIR;

//! FIXME: proto -> int creat(const char * path, mode_t mode)
int creat(const char *path);
int open(const char *path, int oflag, ...);
