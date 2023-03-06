// #include "../include/malloc.h"
#include "../include/ushm.h"
// #include "../include/type.h"
// #include "../include/const.h"
#include "../include/syscall.h"

int totalShmPage;
struct shm_mem shmmen[260];

static void init_shm()
{
    int i,tempaddr;
    totalShmPage = 256;
    tempaddr = ShareLinBase;

    for(i=0;i<totalShmPage;i++,tempaddr+=num_4K)
    {
        shmmen[i].addr = tempaddr;
        shmmen[i].in_use = 0;
    }
}

static  char * shm_vmalloc_4k()
{
    int i;
    for(i=0;i<totalShmPage;i++)
    {
        if(shmmen[i].in_use == 0)
        {
            shmmen[i].in_use == 1;
            return shmmen[i].addr;
        }
    }
    udisp_str("no free shm mem\n");
    return -1;
}

static void shm_free_4k( char * vaddr)
{
    int i;
    for(i=0;i<totalShmPage;i++)
    {
        if(shmmen[i].addr == vaddr)
        {
            shmmen[i].in_use = 0;
        }
    }
}

void *shmat(int shmid, char *shmaddr, int shmflg)
{
    int i;
    int addrlen, index;
    if(shmmen[0].addr == NULL)
    {
        init_shm();
    }
    if (shmaddr == NULL)
    {
        shmaddr = shm_vmalloc_4k();
    }
    else if(shmaddr < shmmen[0].addr || shmaddr > shmmen[totalShmPage -1].addr)
    {
        shmaddr = shm_vmalloc_4k();
    }
    else
    {
        addrlen = shmaddr - shmmen[0].addr;
        index = addrlen / num_4K;
        if(shmmen[index].in_use == 1)
        {
            return shmmen[index].addr;
        }
        else
        {
            shmaddr = shm_vmalloc_4k();
        }
    }

    return _shmat(shmid, shmaddr, shmflg);
}

void shmdt(char *shmaddr)
{
    shm_free_4k(shmaddr);
    _shmdt(shmaddr);
}

