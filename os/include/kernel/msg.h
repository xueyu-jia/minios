/*************************************************************
 *         msg.h  by Li Yingchi Jiao Yuanxiang2021.12.20
 *************************************************************/
#ifndef MSG
#define MSG
#include <kernel/stdint.h>
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

//队列属性
#define MAX_MSQ_NUM 4
#define MAX_MSG_IN_Q 1024
#define MAX_MSGBYTES (1 << 14)

#define ENOENT (-6)
#define EEXIST (-5)
#define ENOMEM (-7)
#define ENOSPC (-8)

// block type
#define SND_TO_FULL 0x1
#define RCV_FROM_NULL 0x2

typedef int key_t;

// message queue types
//队列信息类型
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
} msqid_ds;

//消息类型
typedef struct msg_item {
  long type;
  char* buf;
} msg_item;

//队列中的元素，消息类型在内部，还有前后指针
typedef struct list_item {
  msg_item msg;
  int msgsz;  //消息长度
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
  struct list_item*
      type_list_nxt;  // NOTE: type
                      // 0头结点使用这个指针指向完整队列第一个实际的消息对象！！！
  struct list_item* type_list_pre;
} list_item;

// block queue type
typedef struct block_proc {
  int op;
  int type;
  struct block_proc* nxt;
} block_proc;

//消息队列类型，包括id、key和指向type 0头结点的指针
typedef struct msg_queue {
  int key;
  list_item* head;
  int used;       //是否占用
  int num;        //队列中消息数量
  msqid_ds info;  //队列信息
                  // block queue

  // end
} msg_queue;

// user API:
int ftok(char* f, int key);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int msgctl(int msgqid, int cmd, msqid_ds* buf);

// system call functions:
// PUBLIC int sys_ftok(void* args);
// PUBLIC int sys_msgget(void* args);
// PUBLIC int sys_msgsnd(void* args);
// PUBLIC int sys_msgrcv(void* args);
// PUBLIC int sys_msgctl(void* args);

#endif
