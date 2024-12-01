#include "orangefs_disk.h"

#include <sys/types.h>
#include <unistd.h>

static int disk_fd = -1;

// open device err return -1 otherwise 0
int disk_open(const char *path) {
  disk_fd = open(path, O_RDWR);
  if (disk_fd < 0) {
    return -1;
  }
  return 0;
}

size_t disk_read(size_t block, off_t pos, size_t size, void *buf) {
  lseek(disk_fd, ((block) << BLK_BIT) + pos, SEEK_SET);
  return read(disk_fd, buf, size);
}

size_t disk_write(size_t block, off_t pos, size_t size, void *buf) {
  lseek(disk_fd, ((block) << BLK_BIT) + pos, SEEK_SET);
  size_t b = write(disk_fd, buf, size);
  // fuse_log(FUSE_LOG_DEBUG, "disk write(%ld.%ld, %ld)=%ld\n", block, pos,
  // size, b);
  fsync(disk_fd);
  return b;
}

int disk_close() { return close(disk_fd); }

size_t disk_size() { return lseek(disk_fd, 0, SEEK_END); }