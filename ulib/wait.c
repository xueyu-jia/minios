#include <sys/syscall.h>
#include <sys/wait.h>

pid_t wait(int *wstatus) {
    return syscall(NR_wait, wstatus);
}
