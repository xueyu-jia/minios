#pragma once

#include <sys/types.h>

#define SHM_IPC_CREAT 0x200
#define SHM_IPC_FIND 0x12

#define DELETE 0x41
#define INFO 0x42

typedef struct {
    int size; // 这个共享内存的大小
    int key;  // id 所对应的唯一标识
    int pid;  // 创建进程的 pid
    int state;
    void *phy_address; // 物理页的物理地址
} shm_perm_t;

typedef struct {
    int perms_size;
    int in_use; // 已经被占用的 id 号
    // struct lin_map_id lin_map*;
    shm_perm_t perms[256];
} shm_t;

//! FIXME: proto -> int shmctl(int shmid, int cmd, struct shmid_ds* buf);
shm_t *shmctl(int shmid, int cmd, shm_t *buf);
int shmget(key_t key, size_t size, int flags);
void *shmat(int shmid, const void *addr, int flags);
void shmdt(const void *addr);
void shmcpy(void *dst, const void *src, size_t size);
