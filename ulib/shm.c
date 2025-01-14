#include <sys/shm.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/syscall.h>

typedef struct {
    int addr;
    int in_use;
} shm_mem_t;

static int total_shm_pages;
static shm_mem_t shmmen[260];

static void init_shm() {
    total_shm_pages = 256;
    for (int i = 0, va = ShareLinBase; i < total_shm_pages; ++i, va += SZ_4K) {
        shmmen[i].addr = va;
        shmmen[i].in_use = 0;
    }
}

static void *shm_vmalloc_4k() {
    for (int i = 0; i < total_shm_pages; ++i) {
        if (shmmen[i].in_use == 0) {
            shmmen[i].in_use = 1;
            return (void *)shmmen[i].addr;
        }
    }
    return NULL;
}

static void shm_free_4k(const void *va) {
    for (int i = 0; i < total_shm_pages; ++i) {
        if ((void *)shmmen[i].addr == va) { shmmen[i].in_use = 0; }
    }
}

shm_t *shmctl(int shmid, int cmd, shm_t *buf) {
    return (shm_t *)syscall(NR_shmctl, shmid, cmd, buf);
}

int shmget(key_t key, size_t size, int flags) {
    return syscall(NR_shmget, key, size, flags);
}

void *shmat(int shmid, const void *addr, int flags) {
    int addrlen, index;
    if ((void *)shmmen[0].addr == NULL) { init_shm(); }
    if (addr == NULL) {
        addr = shm_vmalloc_4k();
    } else if (addr < (void *)shmmen[0].addr || addr > (void *)shmmen[total_shm_pages - 1].addr) {
        addr = shm_vmalloc_4k();
    } else {
        addrlen = addr - (void *)shmmen[0].addr;
        index = addrlen / SZ_4K;
        if (shmmen[index].in_use == 1) {
            return (void *)shmmen[index].addr;
        } else {
            addr = shm_vmalloc_4k();
        }
    }
    return (void *)syscall(NR_shmat, shmid, addr, flags);
}

void shmdt(const void *addr) {
    shm_free_4k(addr);
    syscall(NR_shmdt, addr);
}

void shmcpy(void *dst, const void *src, size_t size) {
    syscall(NR_shmcpy, dst, src, size);
}
