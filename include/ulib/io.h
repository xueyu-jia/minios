#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>

int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *path);

char *getcwd(char *buf, ssize_t size);
int chdir(const char *path);
int mkdir(const char *path, mode_t mode);
int rmdir(const char *path);
DIR *opendir(const char *dirname);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);

int mount(const char *src, const char *dst, const char *fs_type, int flags, const void *data);
int umount(const char *dst);
