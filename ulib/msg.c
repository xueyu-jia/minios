#include <sys/msg.h>
#include <sys/syscall.h>

int msgctl(int mq_id, int cmd, msqid_ds_t* buf) {
    return syscall(NR_msgctl, mq_id, cmd, buf);
}

int msgget(key_t key, int flags) {
    return syscall(NR_msgget, key, flags);
}

int msgsnd(int mq_id, const void* msg, int size, int flags) {
    return syscall(NR_msgsnd, mq_id, msg, size, flags);
}

int msgrcv(int mq_id, void* msg, int size, long type, int flags) {
    return syscall(NR_msgrcv, mq_id, msg, size, type, flags);
}
