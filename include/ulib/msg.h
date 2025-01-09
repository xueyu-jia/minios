#pragma once

#include <stdint.h>

#define IPC_CREAT 00001000  //<! create if key is nonexistent
#define IPC_EXCL 00002000   //<! fail if key exists
#define IPC_NOWAIT 00004000 //<! return error on wait

#define MSG_NOERROR 010000 //<! no error if message is too big
#define MSG_EXCEPT 020000  //<! recv any msg except of specified type
#define MSG_COPY 040000    //<! copy (not remove) all queue messages

#define IPC_RMID 0 //<! remove resource
#define IPC_SET 1  //<! set ipc_perm options
#define IPC_STAT 2 //<! get ipc_perm options
#define IPC_INFO 3 //<! see ipcs

#define MAX_MSQ_NUM 4
#define MAX_MSG_IN_Q 1024
#define MAX_MSGBYTES (1 << 14)

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

typedef struct {
    long type;
    uint8_t data[0];
} mq_msg_t;

int ftok(char* f, int key);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int msgctl(int msgqid, int cmd, msqid_ds* buf);
