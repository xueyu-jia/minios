#include <sys/syscall.h>
#include <unistd.h>

pid_t fork() {
    return syscall(NR_fork);
}

pid_t getpid() {
    return syscall(NR_getpid);
}

pid_t getpid_by_name(const char *name) {
    return syscall(NR_getpid_by_name, name);
}
