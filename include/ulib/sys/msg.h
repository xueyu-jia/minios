#pragma once

#include <sys/ipc.h> // IWYU pragma: export
#include <sys/types.h>
#include <stdint.h>

#define MSG_NOERROR 010000 //<! no error if message is too big
#define MSG_EXCEPT 020000  //<! recv any msg except of specified type
#define MSG_COPY 040000    //<! copy (not remove) all queue messages

#define MAX_MSQ_NUM 4
#define MAX_MSG_IN_Q 1024
#define MAX_MSGBYTES (1 << 14)

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
} msqid_ds_t;

typedef struct {
    long type;
    uint8_t data[0];
} mq_msg_t;

int msgctl(int mq_id, int cmd, msqid_ds_t* buf);
int msgget(key_t key, int flags);
int msgsnd(int mq_id, const void* msg, int size, int flags);
int msgrcv(int mq_id, void* msg, int size, long type, int flags);
