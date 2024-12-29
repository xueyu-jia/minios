#pragma once

// msgget使用
#define IPC_CREAT 00001000
#define IPC_EXCL 00002000
// msgsnd和msgrcv使用
#define IPC_NOWAIT 0x1
#define MSG_NOERROR 0x2
// msgctl使用
#define IPC_RMID 0x1
#define IPC_STAT 0x2
#define IPC_SET 0x4

typedef int key_t;

// message queue types
// 队列信息类型
typedef struct msqid_ds {
    // struct ipc_perm msg_perm;   /* Ownership and permissions */ 未实现
    int msg_stime;              /* Time of last msgsnd(2) */
    int msg_rtime;              /* Time of last msgrcv(2) */
    int msg_ctime;              /* Time of last change */
    unsigned long __msg_cbytes; /* Current number of bytes in
                                   queue (nonstandard) */
    int msg_qnum;               /* Current number of messages
                                   in queue */
    int msg_qbytes;             /* Maximum number of bytes
                                   allowed in queue */
    int msg_lspid;              /* PID of last msgsnd(2) */
    int msg_lrpid;              /* PID of last msgrcv(2) */
} msqid_ds;

int ftok(char* f, int key);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int msgctl(int msgqid, int cmd, msqid_ds* buf);
