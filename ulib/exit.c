#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

void exit(int status) {
    _exit(status);
}

void _exit(int status) {
    syscall(NR_exit, status);
    unreachable();
}
