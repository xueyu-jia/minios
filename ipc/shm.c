/************************************************************
********************** add by dingleilei*********************/

#include <minios/clock.h>
#include <minios/console.h>
#include <minios/const.h>
#include <minios/memman.h>
#include <minios/mempage.h>
#include <minios/mmap.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/proto.h>
#include <minios/shm.h>
#include <minios/syscall.h>
#include <minios/type.h>
#include <klib/spinlock.h>
#include <string.h>

struct spinlock lock_shmmemcpy;
void do_shmdt(char *shmaddr);
int do_shmget(int key, int size, int shmflg);
void *do_shmat(int shmid, char *shmaddr, int shmflg);
struct ipc_shm *do_shmctl(int shmid, int cmd, struct ipc_shm *buf);
void *do_shmmemcpy(void *dst, const void *src, long unsigned int len);

// static int alignment(int vir_addr);
static int find_out();
// static char key_available(key);
static int newseg(int key, int shmflg, int size);

struct ipc_shm ids = {.perms_size = 10};
struct spinlock lock_msg = {.locked = 0, .name = "ipc_lock", .cpu = 0xffffffff};
struct spinlock lock_shmmemcpy;
struct address_space shm_pages;

void init_shm() {
    init_mem_page(&shm_pages, MEMPAGE_SAVE);
}

// static char key_available(int key)
// {

// if (ids.perms[key].state == SHM_AVAILABLE)
// {
//     return 1;
// }
// else if (ids.perms[key].state == SHM_BUSY)
// {
//     return 0;
// }
// 	return 0;
// }

static int newseg(int key, int id, int size) {
    ids.in_use++;
    // kprintf("in_use++");
    // ids.seq++;

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
    // kprintf("phy_value: ");
    if (ids.perms[id].phy_address == (void *)-1) // 分配,失败返回-1
    {
        kprintf(" fail mem");
        return -1;
    }

    return id_back;
}

/*======================================================================*
                              shmget
 *======================================================================*/
int find_out() {
    int id = -1;

    int sum = ids.perms_size;
    int i = 0;
    for (; i < sum; i++) {
        if (ids.perms[i].state == SHM_AVAILABLE) {
            id = i;
            break;
        }
    }
    return id;
}

int id_exist(int key) {
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
    acquire(&lock_msg); // 保证get操作的原子性

    if (size > num_4K) {
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

    release(&lock_msg);

    return id;
}

int do_shmget(int key, int size, int shmflg) {
    return kern_shmget(key, size, shmflg);
}

int sys_shmget() {
    return do_shmget(get_arg(1), get_arg(2), get_arg(3));
}

/*======================================================================*
                              shmat
 *======================================================================*/
// PUBLIC int alignment(int vir_addr)
// {
//     int vir = 0;
//     int beh_twe = vir_addr & 0XFFF;
//     if (beh_twe == 0X0)
//     {
//         vir = vir_addr;
//     }
//     else
//     {
//         vir = vir_addr >> 12;
//         vir = vir + 1;
//     }
//     return vir;
// }

void *kern_shmat(int shmid, char *shmaddr, int shmflg) {
    UNUSED(shmflg);
    // u32 vir_addr;
    u32 phy_addr = (u32)ids.perms[shmid].phy_address;
    // // int size = ids.perms[shmid].size;

    // if(phy_addr == NULL)
    // {
    //     ids.perms[shmid].phy_address = (void*)phy_malloc_4k();
    //     phy_addr = (u32)ids.perms[shmid].phy_address;
    // }

    // vir_addr = (u32)shmaddr;

    // alignment(vir_addr);
    // lin_mapping_phy(vir_addr, phy_addr, p_proc_current->task.pid, PG_P |
    // PG_USU | PG_RWW, PG_P | PG_USU | PG_RWW);
    int vir_addr = kern_mmap(p_proc_current, NULL, (u32)shmaddr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shmid);

    if ((void *)phy_addr == NULL) {
        lock_or_yield(&shm_pages.lock);
        memory_page_t *shm_page = find_mem_page(&shm_pages, shmid);
        ids.perms[shmid].phy_address = (void *)pfn_to_phy(page_to_pfn(shm_page));
        release(&shm_pages.lock);
    }
    return (void *)((vir_addr) & 0xFFFFF000);
}

void *do_shmat(int shmid, char *shmaddr, int shmflg) {
    return kern_shmat(shmid, shmaddr, shmflg);
}

void *sys_shmat() {
    return do_shmat(get_arg(1), (char *)get_arg(2), get_arg(3));
}

/*======================================================================*
                            shmdt
 *======================================================================*/
void kern_shmdt(char *shmaddr) {
    // clear_pte(p_proc_current->task.pid, (u32)shmaddr); //并不释放物理页
    kern_munmap(p_proc_current, (u32)shmaddr, PAGE_SIZE);
    return;
}

void do_shmdt(char *shmaddr) {
    return kern_shmdt(shmaddr);
}

void sys_shmdt() {
    return do_shmdt((char *)get_arg(1));
}
/*======================================================================*
                              shmctl
 *======================================================================*/
struct ipc_shm *kern_shmctl(int shmid, int cmd, struct ipc_shm *buf) {
    if (cmd == DELETE) {
        if (ids.perms[shmid].phy_address == NULL || ids.perms[shmid].state == SHM_AVAILABLE) {
            kprintf("memmory has freed before\n");
            return (struct ipc_shm *)buf;
        }
        ids.perms[shmid].state = SHM_AVAILABLE; // 代表这个通道可用
        // int size = ids.perms[shmid].size;
        ids.in_use--;

        phy_free_4k((u32)ids.perms[shmid].phy_address);
        ids.perms[shmid].phy_address = NULL;
    }
    if (cmd == INFO) {
        int shmidsys = do_shmget(ids.in_use + 3, 16, SHM_IPC_CREAT);
        struct ipc_shm *psys = do_shmat(shmidsys, NULL, 0);
        ids.perms[shmidsys].state = SHM_AVAILABLE;
        ids.in_use--;
        buf = psys;

        buf = (struct ipc_shm *)memcpy(buf, &ids, 4096);
    }

    return (struct ipc_shm *)buf;
}

struct ipc_shm *do_shmctl(int shmid, int cmd, struct ipc_shm *buf) {
    return kern_shmctl(shmid, cmd, buf);
}

struct ipc_shm *sys_shmctl() {
    return do_shmctl(get_arg(1), get_arg(2), (struct ipc_shm *)get_arg(3));
}

void *kern_shmmemcpy(void *dst, const void *src, long unsigned int len) {
    acquire(&lock_shmmemcpy);
    if (NULL == dst || NULL == src) {
        release(&lock_shmmemcpy);
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
    release(&lock_shmmemcpy);
    return ret;
}

void *do_shmmemcpy(void *dst, const void *src, long unsigned int len) {
    return kern_shmmemcpy(dst, src, len);
}

void *sys_shmmemcpy() {
    return do_shmmemcpy((void *)get_arg(1), (const void *)get_arg(2), get_arg(3));
}
