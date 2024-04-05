/************************************************************
********************** add by dingleilei*********************/
#ifndef SHM_H
#define SHM_H
#include "spinlock.h"
#include "type.h"
#define SHM_SIZE 10
#define IPC_CREAT 0x200
#define IPC_FIND 0x12

#define DELETE 0x41
#define INFO 0x42

#define NO_EXCLUSION_SERVE 0xFF
#define ERROR 0xFFFFFFFF
#define AVAILABLE 0 //这个宏,不可改
#define BUSY 0x0FFF

struct shm_perm
{
    int size; //这个共享内存的大小
    int key;  //id所对应的唯一标识
    int pid;  //创建进程的pid
    int state;  //共享内存块的可用状态
    void *phy_address; //物理页的物理地址
};
struct ipc_shm
{
    int perms_size;
    int in_use;         //已经被占用的id号
    //struct lin_map_id lin_map*;
    struct shm_perm perms[256];
};


/*
struct key2id_form
{
    char *key2;
    int id;
    struct key2id_form *next;
} Key_TO_Id_Form;*/
extern struct spinlock lock_shmmemcpy;
u32 phy_free_4k(u32 phy_addr);
#endif
