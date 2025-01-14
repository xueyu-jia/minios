#pragma once

#include <minios/cache.h>
#include <minios/memman.h>
#include <klib/stdint.h>

#define SHM_SIZE 10
#define SHM_IPC_CREAT 0x200
#define SHM_IPC_FIND 0x12

#define DELETE 0x41
#define INFO 0x42

#define NO_EXCLUSION_SERVE 0xFF
#define SHM_ERROR 0xFFFFFFFF
#define SHM_AVAILABLE 0 // 这个宏，不可改
#define SHM_BUSY 0x0FFF

typedef struct {
    int size;          // 这个共享内存的大小
    int key;           // id所对应的唯一标识
    int pid;           // 创建进程的pid
    int state;         // 共享内存块的可用状态
    void *phy_address; // 物理页的物理地址
} shm_perm_t;

typedef struct {
    int perms_size;
    int in_use; // 已经被占用的id号
    shm_perm_t perms[256];
} shm_t;

extern spinlock_t lock_shmcpy;
extern address_space_t shm_pages;

void init_shm();

int kern_shmget(int key, int size, int shmflg);
int kern_shmdt(const void *shmaddr);
void *kern_shmat(int shmid, const void *shmaddr, int shmflg);
shm_t *kern_shmctl(int shmid, int cmd, shm_t *buf);
void *kern_shmcpy(void *dst, const void *src, long unsigned int len);
