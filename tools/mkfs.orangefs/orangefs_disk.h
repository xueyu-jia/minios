#include <errno.h>
#include <fcntl.h>
// #include <fuse_log.h>
#define BLK_BIT 12

int disk_open(const char *path);

size_t disk_read(size_t block, off_t pos, size_t size, void *buf);

size_t disk_write(size_t block, off_t pos, size_t size, void *buf);

int disk_close();

size_t disk_size();