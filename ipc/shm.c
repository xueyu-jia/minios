#include <minios/shm.h>
#include <minios/console.h>
#include <minios/mmap.h>
#include <minios/proc.h>
#include <minios/memman.h>
#include <minios/mempage.h>
#include <minios/page.h>
#include <minios/spinlock.h>
#include <minios/syscall.h>
#include <string.h>

spinlock_t lock_shmcpy;

shm_t ids = {.perms_size = 10};
spinlock_t lock_msg = {.locked = 0, .name = "ipc_lock", .cpu = 0xffffffff};
spinlock_t lock_shmcpy;
address_space_t shm_pages;

void init_shm() {
    init_mem_page(&shm_pages, MEMPAGE_SAVE);
}

static int newseg(int key, int id, int size) {
    ids.in_use++;
    if (key < 0 && key >= ids.perms_size) { kprintf("err key"); }
    ids.perms[id].state = SHM_BUSY; // 代表这个shm（share memory）通道被一组id-key占用了
    ids.perms[id].size = size;
    ids.perms[id].pid = p_proc_current->task.pid;
    ids.perms[id].key = key;
    int id_back = id;
    // size = (size / 4096 + 1) * 4096;
    ids.perms[id].phy_address = NULL;

    void *a = ids.perms[id].phy_address;
    UNUSED(a);
    if (ids.perms[id].phy_address == (void *)-1) {
        kprintf(" fail mem");
        return -1;
    }

    return id_back;
}

static int find_out() {
    int id = -1;

    int sum = ids.perms_size;
    int i = 0;
    for (; i < sum; ++i) {
        if (ids.perms[i].state == SHM_AVAILABLE) {
            id = i;
            break;
        }
    }
    return id;
}

static int id_exist(int key) {
    int id = -1;
    int i = 0;
    while (i < SHM_SIZE) {
        if (key == ids.perms[i].key) {
            id = i;
            break;
        }
        i++;
    }

    return id;
}

int kern_shmget(int key, int size, int shmflg) {
    spinlock_acquire(&lock_msg); // 保证get操作的原子性

    if (size > SZ_4K) {
        kprintf("only support 4096 shm mem size\n");
        return -1;
    }
    int id = -1;

    if (shmflg == SHM_IPC_CREAT) { // 1.查看key的是否已经创造来共享内存，如果是，就返回id
                                   // 2.key并没有创建，就找一个空闲的id和key建立关系。
        id = id_exist(key); // 不存在返回-1，存在返回id号

        if (id == -1) {
            id = find_out();
            if (id != -1) {
                id = newseg(key, id, size);
            } // 创建物理页，并建立key-id的关系
            else {
                kprintf("\nno_free_shm");
            }
        }
    }
    if (shmflg == SHM_IPC_FIND) // 寻找key试图与之链接，key如果没有创建物理内存，就返回-1;
    {
        // kprintf("FIND");
        id = id_exist(key); // 不存在返回-1，存在返回id号
        if (id == -1) { kprintf("\nthis key possessed"); }
    }
    if ((shmflg != SHM_IPC_FIND) && (shmflg != SHM_IPC_CREAT)) { kprintf("need_priority"); }

    spinlock_release(&lock_msg);

    return id;
}

void *kern_shmat(int shmid, const void *shmaddr, int shmflg) {
    UNUSED(shmflg);
    u32 phy_addr = (u32)ids.perms[shmid].phy_address;
    int vir_addr = kern_mmap(p_proc_current, NULL, (u32)shmaddr, PGSIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shmid);

    if ((void *)phy_addr == NULL) {
        spinlock_lock_or_yield(&shm_pages.lock);
        memory_page_t *shm_page = find_mem_page(&shm_pages, shmid);
        ids.perms[shmid].phy_address = (void *)pfn_to_phy(page_to_pfn(shm_page));
        spinlock_release(&shm_pages.lock);
    }
    return (void *)((vir_addr) & 0xfffff000);
}

int kern_shmdt(const void *shmaddr) {
    //! FIXME: unhandled retval
    kern_munmap(p_proc_current, (u32)shmaddr, PGSIZE);
    return 0;
}

shm_t *kern_shmctl(int shmid, int cmd, shm_t *buf) {
    if (cmd == DELETE) {
        if (ids.perms[shmid].phy_address == NULL || ids.perms[shmid].state == SHM_AVAILABLE) {
            kprintf("memmory has freed before\n");
            return (shm_t *)buf;
        }
        ids.perms[shmid].state = SHM_AVAILABLE; // 代表这个通道可用
        // int size = ids.perms[shmid].size;
        ids.in_use--;

        phy_free_4k((u32)ids.perms[shmid].phy_address);
        ids.perms[shmid].phy_address = NULL;
    }
    if (cmd == INFO) {
        int shmidsys = do_shmget(ids.in_use + 3, 16, SHM_IPC_CREAT);
        shm_t *psys = do_shmat(shmidsys, NULL, 0);
        ids.perms[shmidsys].state = SHM_AVAILABLE;
        ids.in_use--;
        buf = psys;

        buf = (shm_t *)memcpy(buf, &ids, 4096);
    }

    return (shm_t *)buf;
}

void *kern_shmcpy(void *dst, const void *src, long unsigned int len) {
    spinlock_acquire(&lock_shmcpy);
    if (NULL == dst || NULL == src) {
        spinlock_release(&lock_shmcpy);
        return NULL;
    }

    void *ret = dst;

    if (dst <= src || (char *)dst >= (char *)src + len) {
        // 没有内存重叠，从低地址开始复制
        while (len--) {
            // sleep(100);
            *(char *)dst = *(char *)src;
            dst = (char *)dst + 1;
            src = (char *)src + 1;
        }
    } else {
        // 有内存重叠，从高地址开始复制
        src = (char *)src + len - 1;
        dst = (char *)dst + len - 1;
        while (len--) {
            *(char *)dst = *(char *)src;
            dst = (char *)dst - 1;
            src = (char *)src - 1;
        }
    }
    spinlock_release(&lock_shmcpy);
    return ret;
}
