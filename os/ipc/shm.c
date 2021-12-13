/************************************************************
********************** add by dingleilei*********************/

#include "../include/shm.h"
#include "../include/stdio.h"
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/spinlock.h"

PUBLIC void do_shmdt(char *shmaddr);
PUBLIC int do_shmget(int key, int size, int shmflg);
PUBLIC void *do_shmat(int shmid, char *shmaddr, int shmflg);
PUBLIC struct ipc_shm *do_shmctl(int shmid, int cmd, struct ipc_shm *buf);
PUBLIC void *do_shmmemcpy(void *dst, const void *src, long unsigned int len);

static int alignment(int vir_addr);
static int find_out();
static char key_available(key);
static int newseg(int key, int shmflg, int size);

PUBLIC struct ipc_shm ids = {.perms_size = 10};
PUBLIC struct spinlock lock_msg = {
    .locked = 0,
    .name = "ipc_lock",
    .cpu = 0xffffffff};


static char key_available(key)
{

    if (ids.perms[key].state == AVAILABLE)
    {
        return 1;
    }
    else if (ids.perms[key].state == BUSY)
    {
        return 0;
    }
}


static int newseg(int key, int id, int size)
{
    ids.in_use++;
    //disp_str("in_use++");
    // ids.seq++;

    if (key < 0 && key >= ids.perms_size)
    {
        disp_str("err key");
    }
    ids.perms[id].state = BUSY; //代表这个shm（share memory）通道被一组id-key占用了
    ids.perms[id].size = size;
    ids.perms[id].pid = p_proc_current->task.pid;
    ids.perms[id].key = key;
    int id_back = id;
    //disp_int(id_back);
    // size = (size / 4096 + 1) * 4096;
    ids.perms[id].phy_address = NULL;

    //disp_int( ids.perms[id].phy_address);

    void* a=ids.perms[id].phy_address;
	// disp_str("phy_value: ");
	// disp_int(a);//打印物理地址
    if (ids.perms[id].phy_address == -1) //分配,失败返回-1
    {
        disp_str(" fail mem");
        return -1;
    }

    return id_back;
}

/*======================================================================*
                              shmget
 *======================================================================*/
PUBLIC int find_out()
{
    int id = -1;

    int sum = ids.perms_size;
    int i = 0;
    for (; i < sum; i++)
    {
        if (ids.perms[i].state == AVAILABLE)
        {
            id = i;
            break;
        }
    }
    return id;
}

PUBLIC int id_exist(int key)
{
    int id = -1;
    int i = 0;
    while (i < SHM_SIZE)
    {
        if (key == ids.perms[i].key)
        {
            id = i;
            break;
        }
        i++;
    }

    return id;
}

PUBLIC int do_shmget(int key, int size, int shmflg)
{
    acquire(&lock_msg); //保证get操作的原子性


    if(size > num_4K)
    {
        disp_str("only support 4096 shm mem size\n");
        return -1;
    }
    int id = -1;

    if (shmflg == IPC_CREAT)
    {                      //1.查看key的是否已经创造来共享内存，如果是，就返回id
                           //2.key并没有创建，就找一个空闲的id和key建立关系。
        id = id_exist(key); //不存在返回-1，存在返回id号

        if (id == -1)
        {
            id = find_out();
            if (id != -1)
            {
                id = newseg(key, id, size);
            } //创建物理页，并建立key-id的关系
            else
            {
                disp_str("\nno_free_shm");
            }
        }

        

        //disp_int(id);
    }
    if (shmflg == IPC_FIND) //寻找key试图与之链接，key如果没有创建物理内存，就返回-1;
    {
        //disp_str("FIND");
        id = id_exist(key); //不存在返回-1，存在返回id号
        if (id == -1)
        {
            disp_str("\nthis key possessed");
        }
    }
    if ((shmflg != IPC_FIND) && (shmflg != IPC_CREAT))
    {
        disp_str("need_priority");
    }

    release(&lock_msg);

    return id;
}

PUBLIC int sys_shmget(void *uesp)
{  
    return do_shmget(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}


/*======================================================================*
                              shmat
 *======================================================================*/
PUBLIC int alignment(int vir_addr)
{
    int vir = 0;
    int beh_twe = vir_addr & 0XFFF;
    if (beh_twe == 0X0)
    {
        vir = vir_addr;
    }
    else
    {
        vir = vir_addr >> 12;
        vir = vir + 1;
    }
    return vir;
}

PUBLIC void *do_shmat(int shmid, char *shmaddr, int shmflg)
{

    int vir_addr;
    int phy_addr = ids.perms[shmid].phy_address;
    // int size = ids.perms[shmid].size;

    if(phy_addr == NULL)
    {
        ids.perms[shmid].phy_address = phy_malloc_4k();
        phy_addr = ids.perms[shmid].phy_address;
    }
    
    vir_addr = shmaddr;
    
    // alignment(vir_addr);
    lin_mapping_phy(vir_addr, phy_addr, p_proc_current->task.pid, PG_P | PG_USU | PG_RWW, PG_P | PG_USU | PG_RWW);

    return (void *)((vir_addr)&0xFFFFF000);
}

PUBLIC void *sys_shmat(void *uesp)
{
    return do_shmat(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

/*======================================================================*
                            shmdt 
 *======================================================================*/
PUBLIC void do_shmdt(char *shmaddr)
{
    clear_pte(p_proc_current->task.pid, shmaddr); //并不释放物理页

    return;
}

PUBLIC void sys_shmdt(void *uesp)
{
    return do_shmdt(get_arg(uesp, 1));
}
/*======================================================================*
                              shmctl
 *======================================================================*/

PUBLIC struct ipc_shm *do_shmctl(int shmid, int cmd, struct ipc_shm *buf)
{
    if (cmd == DELETE)
    {
        if(ids.perms[shmid].phy_address == NULL || ids.perms[shmid].state == AVAILABLE)
        {
            disp_str("memmory has freed before\n");
            return (struct ipc_shm *)buf;
        }
        ids.perms[shmid].state = AVAILABLE; //代表这个通道可用
        // int size = ids.perms[shmid].size;
        ids.in_use--;

        phy_free_4k(ids.perms[shmid].phy_address);
        ids.perms[shmid].phy_address = NULL;
    }
    if (cmd == INFO)
    {
        int shmidsys = do_shmget(ids.in_use + 3, 16, IPC_CREAT);
        struct ipc_shm *psys = do_shmat(shmidsys, NULL, 0);
        ids.perms[shmidsys].state = AVAILABLE;
        ids.in_use--;
        buf = psys;

        buf = (struct ipc_shm *)memcpy(buf, &ids, 4096);
        disp_str("\ninfo_in_kernel: ");
        disp_str("ids.in_use: ");
        disp_int(ids.in_use);
    }

    return (struct ipc_shm *)buf;
}

PUBLIC struct ipc_shm *sys_shmctl(void *uesp)
{
    return do_shmctl(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

PUBLIC void *sys_shmmemcpy(void *uesp)
{
    return do_shmmemcpy(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

PUBLIC void *do_shmmemcpy(void *dst, const void *src, long unsigned int len)
{
    acquire(&lock_shmmemcpy);
    if (NULL == dst || NULL == src)
    {
        release(&lock_shmmemcpy);
        return NULL;
    }

    void *ret = dst;

    if (dst <= src || (char *)dst >= (char *)src + len)
    {
        //没有内存重叠，从低地址开始复制
        while (len--)
        {
            //sleep(100);
            *(char *)dst = *(char *)src;
            dst = (char *)dst + 1;
            src = (char *)src + 1;
        }
    }
    else
    {
        //有内存重叠，从高地址开始复制
        src = (char *)src + len - 1;
        dst = (char *)dst + len - 1;
        while (len--)
        {
            *(char *)dst = *(char *)src;
            dst = (char *)dst - 1;
            src = (char *)src - 1;
        }
    }
    release(&lock_shmmemcpy);
    return ret;
}