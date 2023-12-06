#ifndef USHM_H
#define USHM_H

#define SHM_IPC_CREAT 0x200
#define SHM_IPC_FIND 0x12

#define DELETE 0x41
#define INFO 0x42

struct shm_perm
{
    int size; //这个共享内存的大小
    int key;  //id所对应的唯一标识
    int pid;  //创建进程的pid
    int state;
    void *phy_address; //物理页的物理地址
};
struct ipc_shm
{
    int perms_size;
    int in_use;         //已经被占用的id号
    //struct lin_map_id lin_map*;
    struct shm_perm perms[256];
};

struct shm_mem
{
    int addr;
    int in_use;
};


void *shmat(int shmid, char *shmaddr, int shmflg);
void shmdt(char *shmaddr);

#endif