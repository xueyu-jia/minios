#pragma once

#include <type.h>

extern int (*read)(u32 offset, u32 lenth, void *buf);
extern int (*open_file)(char *filename);

int init_fs();
