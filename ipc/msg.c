#include <minios/msg.h>
#include <minios/clock.h>
#include <minios/memman.h>
#include <minios/msg.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/assert.h>
#include <minios/sched.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

msg_queue q_list[MAX_MSQ_NUM];

void pop_front_msg_node(int msqid, list_item *head);
void write_msg_node(int msqid, list_item *node, int msgsz, const mq_msg_t *msg_ptr);
int newque(int id, key_t key);
int ipc_findkey(key_t key);
list_item *insert_head_node(int msqid, long type_new);
int insert_type_list(int msqid, list_item *new_msg);
int insert_full_list(int msqid, list_item *new_msg);

void init_msgq() {
    int i;
    for (i = 0; i < MAX_MSQ_NUM; ++i) {
        q_list[i].key = 0;
        q_list[i].used = 0;    // set queue unuesd
        q_list[i].head = NULL; // set head pointer NULL
    }
}

int kern_ftok(const char *pathname, int proj_id) {
    unsigned int key = proj_id;
    const char *s = pathname;
    for (int base = 1; *s; ++s) {
        key += ((int)*s) * base;
        base *= 233;
    }
    return (int)(key & 0xefffffff);
}

// real functions
/**
 * @brief 根据key创建一个消息队列，或找到已存在的某个消息队列
 *
 * @param key 		键值key，应该是ftok()的返回值
 * @param msgflg 	可用值：
 * 					0：必须返回已存在的队列或错误码
 * 					IPC_CREAT：允许新建队列
 * 					IPC_EXCL：返回的队列必须是新建的，需要与IPC_CREAT连用
 * @return int 		消息队列ID
 */
int kern_msgget(key_t key, int msgflg) {
    const int id = ipc_findkey(key);
    if (id < 0) {
        if (!(msgflg & IPC_CREAT)) { return -ENOENT; }
        for (int i = 0; i < MAX_MSQ_NUM; ++i) {
            if (q_list[i].used) { continue; }
            newque(i, key);
            return i;
        }
        return -ENOSPC;
    }
    if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
        return -EEXIST;
    } else {
        // TODO: check perm, p/v lock and so on
        return id;
    }
    return -1;
}

/*!
 * \see https://man7.org/linux/man-pages/man3/msgsnd.3p.html
 */
int kern_msgsnd(int msqid, const void *msgp, int msgsz, int msgflg) {
    if (!(msqid >= 0 && msqid < MAX_MSQ_NUM && q_list[msqid].used)) { return -1; /* EINVAL */ }
    if (msgp == NULL) { return -1; }
    if (msgsz < 0) { return -1; }

    const mq_msg_t *msg_ptr = msgp;
    if (msg_ptr->type <= 0) { return -1; /* EINVAL */ }

    //! FIXME: concurrency unsafe

    while (true) {
        if (q_list[msqid].info.__msg_cbytes + msgsz <= q_list[msqid].info.msg_qbytes) { break; }
        if (msgflg & IPC_NOWAIT) { return -1; /* EAGAIN */ }
        wait_event(&q_list[msqid]);
        if (!q_list[msqid].used) { return -1; /* EIDRM */ }
    }

    list_item *new_msg_node = kern_kmalloc(sizeof(list_item));
    write_msg_node(msqid, new_msg_node, msgsz, msg_ptr);
    insert_head_node(msqid, msg_ptr->type);

    const bool tl_inserted = insert_type_list(msqid, new_msg_node) == 0;
    const bool fl_inserted = insert_full_list(msqid, new_msg_node) == 0;
    assert(tl_inserted && fl_inserted);

    wakeup(&q_list[msqid]);
    return 0;
}

static list_item *msgrcv_try_get_first_msg(list_item *head, int msgtyp) {
    assert(msgtyp == 0);
    return head;
}

static list_item *msgrcv_try_get_expected_msg(list_item *head, int msgtyp) {
    assert(msgtyp > 0);
    list_item *node = head->nxt.head_nxt;
    while (node) {
        if (node->msg.type == msgtyp) { return node; }
        node = node->nxt.head_nxt;
    }
    return NULL;
}

static list_item *msgrcv_try_get_neg_msg(list_item *head, int msgtyp) {
    assert(msgtyp < 0);
    list_item dummy = {};
    dummy.msg.type = LONG_MAX;
    list_item *min_msgtyp_node = &dummy;
    list_item *node = head->nxt.head_nxt;
    while (node) {
        if (node->msg.type <= -msgtyp && node->msg.type < min_msgtyp_node->msg.type) {
            min_msgtyp_node = node;
        }
        node = node->nxt.head_nxt;
    }
    return min_msgtyp_node == &dummy ? NULL : min_msgtyp_node;
}

static list_item *msgrcv_try_get_except_msg(list_item *head, int msgtyp) {
    assert(msgtyp > 0);
    list_item *node = head->nxt.head_nxt;
    while (node) {
        if (node->msg.type != msgtyp) { return node; }
        node = node->nxt.head_nxt;
    }
    return NULL;
}

/*!
 * \see https://man7.org/linux/man-pages/man3/msgrcv.3p.html
 */
int kern_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg) {
    if (!(msqid >= 0 && msqid < MAX_MSQ_NUM && q_list[msqid].used)) { return -1; /* EINVAL */ }
    if (msgp == NULL) { return -1; }
    if (msgsz < 0) { return -1; }

    mq_msg_t *msg_ptr = msgp;

    //! FIXME: concurrency unsafe

    list_item *(*try_get_msg)(list_item *, int) = NULL;
    if (false) {
    } else if (msgtyp == 0) {
        try_get_msg = msgrcv_try_get_first_msg;
    } else if (msgtyp < 0) {
        try_get_msg = msgrcv_try_get_neg_msg;
    } else if (msgflg & MSG_EXCEPT) {
        try_get_msg = msgrcv_try_get_except_msg;
    } else {
        try_get_msg = msgrcv_try_get_expected_msg;
    }

    //! TODO: separate recv & send wakeup channel

    //! NOTE: msgq can be deleted in another proc
    while (q_list[msqid].used) {
        list_item *head_node = try_get_msg(q_list[msqid].head, msgtyp);
        if (head_node != NULL && head_node->type_list_nxt) {
            assert(q_list[msqid].num > 0);
            const list_item *msg_node = head_node->type_list_nxt;
            msg_ptr->type = msg_node->msg.type;
            const bool need_truncate = msg_node->msgsz > msgsz;
            if (need_truncate && !(msgflg & MSG_NOERROR)) { return -1; /* E2BIG */ }
            const int recv_len = MIN(msgsz, msg_node->msgsz);
            memcpy(msg_ptr->data, msg_node->msg.buf, recv_len);
            pop_front_msg_node(msqid, head_node);
            --q_list[msqid].num;
            wakeup(&q_list[msqid]);
            return recv_len;
        }
        if (msgflg & IPC_NOWAIT) { return -1; /* ENOMSG */ }
        wait_event(&q_list[msqid]);
    }
    return -1; /* EIDRM */
}

/**
 * @brief 			对消息队列进行某些操作
 *
 * @param msqid 	操作的队列ID
 * @param cmd 		指定操作类型，取值和意义如下：
 * 					IPC_STAT：将队列信息拷贝到buf中
 * 					IPC_SET：修改队列信息，将buf的数据写入队列中（目前只能修改最大字节数）
 * 					IPC_RMID：删除队列，删除时应该唤醒所有阻塞队列上的进程（实现）
 * 					IPC_INFO：?
 * 					MSG_INFO：?
 * 					MSG_STAT：?
 * @param buf
 * 存放队列信息结构的指针，buf 应该是 msqid_ds_t 类型，这个类型还没有定义
 * @return int 		对前三个cmd值，操作成功返回0，出错返回-1
 */
int kern_msgctl(int msqid, int cmd, msqid_ds_t *buf) {
    if (msqid < 0 || msqid > MAX_MSQ_NUM || !q_list[msqid].used) { return -1; }
    switch (cmd) {
        case IPC_RMID: {
            list_item *i_head = q_list[msqid].head->nxt.head_nxt;
            if (q_list[msqid].info.msg_qnum > 0) {
                while (i_head) {
                    while (i_head->type_list_nxt) pop_front_msg_node(msqid, i_head);
                    i_head = i_head->nxt.head_nxt;
                }
            }
            i_head = q_list[msqid].head;
            while (i_head) {
                list_item *temp_nxt = i_head->nxt.head_nxt;
                kern_kfree(i_head);
                i_head = temp_nxt;
            }
            q_list[msqid].used = 0;
            q_list[msqid].head = NULL;
            wakeup(&q_list[msqid]);
            return 0;
        } break;
        case IPC_STAT: {
            if (buf == NULL) { break; }
            *(msqid_ds_t *)buf = q_list[msqid].info;
            return 0;
        } break;
        case IPC_SET: {
            if (buf == NULL) { break; }
            q_list[msqid].info.msg_qbytes = (*(msqid_ds_t *)buf).msg_qbytes;
            q_list[msqid].info.msg_ctime = kern_getticks();
            return 0;
        } break;
    }
    return -1;
}

// utils:
/**
 * @brief 			从队列中取出删除一个消息，同时修改队列的信息
 *
 * @param msqid 	队列ID
 * @param head 		头结点指针
 */
void pop_front_msg_node(int msqid, list_item *head) {
    if (!head) // 输入指针为NULL，直接返回
        return;
    list_item *target = head->type_list_nxt;
    if (!target) // haed后面没有任何消息结点
        return;
    int size = target->msgsz; // 要删除的消息的长度
    list_item *f_nxt = target->nxt.full_list_nxt;
    list_item *f_pre = target->pre.full_list_pre;
    list_item *t_nxt = target->type_list_nxt;
    head->type_list_nxt = t_nxt;
    if (t_nxt) t_nxt->type_list_pre = head;
    if (f_nxt) f_nxt->pre.full_list_pre = f_pre;
    if (f_pre == q_list[msqid].head)
        q_list[msqid].head->type_list_nxt = f_nxt;
    else
        f_pre->nxt.full_list_nxt = f_nxt;
    // 释放target内存
    kern_kfree(target->msg.buf);
    kern_kfree(target);
    // 修改队列信息
    q_list[msqid].info.msg_lrpid = kern_getpid();
    q_list[msqid].info.msg_qnum--;
    q_list[msqid].info.__msg_cbytes -= size;
    q_list[msqid].info.msg_rtime = kern_getticks();
    return;
}

/**
 * @brief 添加消息时，将消息内容写入队列中的结点，并修改队列的一些信息
 *
 * @param msqid 	队列ID
 * @param node 		结点指针，应分配空间后再调用此函数
 * @param msgsz 	消息长度，由用户提供
 * @param msg_ptr 	消息项
 */
void write_msg_node(int msqid, list_item *node, int msgsz, const mq_msg_t *msg_ptr) {
    assert(msg_ptr != NULL);
    assert(msgsz >= 0);
    ++q_list[msqid].num;
    ++q_list[msqid].info.msg_qnum;
    q_list[msqid].info.__msg_cbytes += msgsz;
    q_list[msqid].info.msg_lspid = kern_getpid();
    q_list[msqid].info.msg_stime = kern_getticks();
    node->msg.type = msg_ptr->type;
    node->msg.buf = kern_kmalloc(MAX(msgsz, 1)); //<! alloc at least 1 byte for convenience
    node->msgsz = msgsz;
    memcpy(node->msg.buf, msg_ptr->data, msgsz);
    return;
}

/**
 * @brief 			初始化q_list的第id个队列，键值设为key
 *
 * @param id 		q_list的位置，调用前应保证第id个队列是空闲的
 * @param key 		键值，初始化的队列键值会被设为key
 * @return int 		成功返回0，否则返回-1
 */
int newque(int id, key_t key) {
    if (id < 0 || id >= MAX_MSQ_NUM) return -1;
    memset(&q_list[id], 0, sizeof(msg_queue));
    list_item *ph = kern_kmalloc(sizeof(list_item));
    memset(ph, 0, sizeof(list_item));
    q_list[id].used = 1;
    q_list[id].head = ph;
    q_list[id].info.msg_qbytes = MAX_MSGBYTES;
    q_list[id].key = key;
    return 0;
}

/**
 * @brief 			在q_list中寻找是否有存在的队列键值等于key
 *
 * @param key 		查询的key
 * @return int 		若有返回队列id，否则返回-1
 */
int ipc_findkey(key_t key) {
    int id;
    for (id = 0; id < MAX_MSQ_NUM; ++id) {
        if (q_list[id].key == key && q_list[id].used) return id;
    }
    return -1;
}

/**
 * @brief 				插入一个指定类型的头结点，可能分配内存
 *
 * @param msqid 		消息队列ID，向哪个队列插入
 * @param type_new 		新的头结点类型
 * @return list_item* 	返回新插入的头结点指针，如果该类型头结点已存在，返回NULL
 */
list_item *insert_head_node(int msqid, long type_new) {
    // 创建新的空头结点
    list_item *new_head = kern_kmalloc(sizeof(list_item));
    memset(new_head, 0, sizeof(list_item));
    new_head->msg.type = type_new;
    // 检查该类型是否存在
    list_item *i = NULL;
    for (i = q_list[msqid].head; i && i->msg.type <= type_new; i = i->nxt.head_nxt) {
        if (i->msg.type == type_new) {
            kern_kfree(new_head);
            return NULL;
        }
    }
    if (i) {
        list_item *pre = i->pre.head_pre;
        pre->nxt.head_nxt = new_head;
        new_head->pre.head_pre = pre;
        new_head->nxt.head_nxt = i;
        i->pre.head_pre = new_head;
    } else {
        for (i = q_list[msqid].head; i->nxt.head_nxt; i = i->nxt.head_nxt);
        i->nxt.head_nxt = new_head;
        new_head->pre.head_pre = i;
    }
    return new_head;
}

/**
 * @brief 			将一个消息结点插入类型链，必须保证已经初始化
 *
 * @param msqid 	消息队列ID
 * @param new_msg 	待插入消息结点指针
 * @return int		成功返回0，否则返回-1
 */
int insert_type_list(int msqid, list_item *new_msg) {
    if (new_msg->msg.type <= 0) return -1;
    list_item *head = NULL;
    for (head = q_list[msqid].head; head; head = head->nxt.head_nxt) {
        if (head->msg.type == new_msg->msg.type) break;
    }
    if (head) {
        list_item *t = head;
        while (t->type_list_nxt) t = t->type_list_nxt;
        t->type_list_nxt = new_msg;
        new_msg->type_list_pre = t;
        new_msg->type_list_nxt = NULL;
    }
    return 0;
}

/**
 * @brief 			将一个消息结点插入总链，必须保证已经初始化
 *
 * @param msqid 	消息队列ID
 * @param new_msg 	待插入消息结点指针
 * @return int		成功返回0，否则返回-1
 */
int insert_full_list(int msqid, list_item *new_msg) {
    if (new_msg->msg.type <= 0) return -1;
    list_item *i = NULL;
    for (i = q_list[msqid].head->type_list_nxt; i && i->nxt.full_list_nxt;
         i = i->nxt.full_list_nxt);
    if (!i) {
        q_list[msqid].head->type_list_nxt = new_msg;
        new_msg->pre.full_list_pre = q_list[msqid].head;
    } else {
        i->nxt.full_list_nxt = new_msg;
        new_msg->pre.full_list_pre = i;
    }
    new_msg->nxt.full_list_nxt = NULL;
    return 0;
}
