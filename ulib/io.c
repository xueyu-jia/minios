#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdarg.h>

int creat(const char *path, mode_t mode) {
    return open(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}

int open(const char *path, int oflag, ...) {
    mode_t mode = 0;
    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    return syscall(NR_open, path, oflag, mode);
}

ssize_t read(int fd, void *buf, size_t count) {
    return syscall(NR_read, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return syscall(NR_write, fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence) {
    return syscall(NR_lseek, fd, offset, whence);
}

int close(int fd) {
    return syscall(NR_close, fd);
}

int unlink(const char *path) {
    return syscall(NR_unlink, path);
}

char *getcwd(char *buf, ssize_t size) {
    return (char *)syscall(NR_getcwd, buf, size);
}

int chdir(const char *path) {
    return syscall(NR_chdir, path);
}

int mkdir(const char *path, mode_t mode) {
    return syscall(NR_mkdir, path, mode);
}

int rmdir(const char *path) {
    return syscall(NR_rmdir, path);
}

DIR *opendir(const char *path) {
    return (DIR *)syscall(NR_opendir, path);
}

int closedir(DIR *dir) {
    return syscall(NR_closedir, dir);
}

struct dirent *readdir(DIR *dir) {
    return (struct dirent *)syscall(NR_readdir, dir);
}

int mount(const char *src, const char *dst, const char *fs_type, int flags, const void *data) {
    return syscall(NR_mount, src, dst, fs_type, flags, data);
}

int umount(const char *dst) {
    return syscall(NR_umount, dst);
}

int stat(const char *path, struct stat *buf) {
    return syscall(NR_stat, path, buf);
}
