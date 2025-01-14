#pragma once

#include <sys/types.h>
#include <klib/stdint.h>

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

// 队列属性
#define MAX_MSQ_NUM 4
#define MAX_MSG_IN_Q 1024
#define MAX_MSGBYTES (1 << 14)

// block type
#define SND_TO_FULL 0x1
#define RCV_FROM_NULL 0x2

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
    u32 msg_qbytes;             /* Maximum number of bytes
                                   allowed in queue */
    int msg_lspid;              /* PID of last msgsnd(2) */
    int msg_lrpid;              /* PID of last msgrcv(2) */
} msqid_ds_t;

// 消息类型
typedef struct msg_item {
    long type;
    char* buf;
} msg_item;

typedef struct {
    long type;
    uint8_t data[0];
} mq_msg_t;

// 队列中的元素，消息类型在内部，还有前后指针
typedef struct list_item {
    msg_item msg;
    int msgsz; // 消息长度
    /**
     * 这两个联合体是结点在整个链表中的使用的，头结点使用head_nxt和head_pre，
     * 非头结点使用full_list_nxt和full_list_pre
     * 头结点的type_list_pre都应该为NULL
     */
    union {
        struct list_item* full_list_nxt;
        struct list_item* head_nxt;
    } nxt;
    union {
        struct list_item* full_list_pre;
        struct list_item* head_pre;
    } pre;

    /**
     * 这两个指针是结点在自己类型队列中使用的，标记在类型队列中的前后结点
     */
    struct list_item* type_list_nxt; // NOTE: type
                                     // 0头结点使用这个指针指向完整队列第一个实际的消息对象！！！
    struct list_item* type_list_pre;
} list_item;

// block queue type
typedef struct block_proc {
    int op;
    int type;
    struct block_proc* nxt;
} block_proc;

// 消息队列类型，包括id、key和指向type 0头结点的指针
typedef struct msg_queue {
    int key;
    list_item* head;
    int used;        // 是否占用
    int num;         // 队列中消息数量
    msqid_ds_t info; // 队列信息
                     //  block queue
    // end
} msg_queue;

void init_msgq();

int kern_ftok(const char* pathname, int proj_id);
int kern_msgget(key_t key, int msgflg);
int kern_msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int kern_msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int kern_msgctl(int msgqid, int cmd, msqid_ds_t* buf);
