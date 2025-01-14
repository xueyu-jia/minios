#include <sys/ipc.h>
#include <sys/syscall.h>

key_t ftok(const char *path, int id) {
    return syscall(NR_ftok, path, id);
}
