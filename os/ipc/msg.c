/*************************************************************
 *        msg.c by Li Yingchi Jiao Yuanxiang 2021.12.20
 *************************************************************/
#include "type.h"
#include "const.h"
#include "console.h"
#include "clock.h"
#include "string.h"
#include "proc.h"
#include "memman.h"
#include "pagetable.h"
#include "msg.h"
#include "proto.h"

// instances
msg_queue q_list[MAX_MSQ_NUM];

// declarations of do functions:
int kern_msgget(key_t key, int msgflg);
int kern_msgsnd(int msqid, const void *msgp, int msgsz, int msgflg);
int kern_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg);
int kern_msgctl(int msgqid, int cmd, msqid_ds *buf);

// other
void rm_msg_node(int msqid, list_item *head);
void write_msg_node(int msqid, list_item *node, int msgsz, char *msgp, long type_new);
int newque(int id, key_t key);
int ipc_findkey(key_t key);
list_item *insert_head_node(int msqid, long type_new);
int insert_type_list(int msqid, list_item *new_msg);
int insert_full_list(int msqid, list_item *new_msg);

/*************************************************************************
*	初始化消息队列 		added by yinchi 2021.12.24
***************************************************************************/
void init_msgq(){
	int i;
	for(i=0;i<MAX_MSQ_NUM;i++){
		q_list[i].key = 0;
		q_list[i].used = 0;//set queue unuesd
		q_list[i].head = NULL;//set head pointer NULL
	}
	disp_str("Message queue initialization done.\n");
}
// functions in system call table:
int kern_ftok(char* f, int flag)
{
	unsigned int key = flag;
	for (int base = 1; *f; f++)
	{
		key += ((int)*f) * base;
		base *= 233;
	}
	return (int)(key & 0xEFFFFFFF);
}

int do_ftok(char* f, int flag)
{
	return kern_ftok(f, flag);
}

int sys_ftok()
{
	return do_ftok((char*)get_arg(1), get_arg(2));
}

int do_msgget(key_t key, int msgflg )
{
	return kern_msgget(key, msgflg);
}

int sys_msgget()
{
	return do_msgget(get_arg(1), get_arg(2));
}


int do_msgsnd(int msqid, const void *msgp, int msgsz, int msgflg)
{
	return kern_msgsnd(msqid, msgp, msgsz, msgflg);
}

int sys_msgsnd()
{
	return do_msgsnd(get_arg(1), (const void*)get_arg(2), get_arg(3), get_arg(4));
}


int do_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg)
{
	return kern_msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
}

int sys_msgrcv()
{
	return do_msgrcv(get_arg(1), (void*)get_arg(2), get_arg(3), get_arg(4), get_arg(5));
}


int do_msgctl(int msqid, int cmd, msqid_ds *buf)
{
	return kern_msgctl(msqid, cmd, buf);
}

int sys_msgctl()
{
	return do_msgctl(get_arg(1), get_arg(2), (msqid_ds *)get_arg(3));
}

// real functions
/**
 * @brief 			根据key创建一个消息队列，或找到已存在的某个消息队列
 *
 * @param key 		键值key，应该是ftok()的返回值
 * @param msgflg 	可用值：
 * 					0：必须返回已存在的队列或错误码
 * 					IPC_CREAT：允许新建队列
 * 					IPC_EXCL：返回的队列必须是新建的，需要与IPC_CREAT连用
 * @return int 		消息队列ID
 */
int kern_msgget(key_t key, int msgflg)
{
	int i;
	int id = ipc_findkey(key);
	if (id < 0)
	{ // key不存在
		if (msgflg & IPC_CREAT)
		{
			for (i = 0; i < MAX_MSQ_NUM; i++) //寻找空闲队列
			{
				if (!q_list[i].used)
				{
					newque(i, key);
					return i;
				}
			}
			return ENOSPC; //-ENOSPC
		}
		else
			return ENOENT; //-ENOENT
	}
	else
	{
		if (msgflg & IPC_CREAT && msgflg & IPC_EXCL)
			return EEXIST; //-EEXIST
		else
		{
			//检查权限、PV操作等……
			return id;
		}
	}
	return -1;
}

/**
 * @brief 			向消息队列中发送一条消息
 *
 * @param msqid 	队列ID
 * @param msgp 		消息缓冲区，消息结构指针，必须保证开头是一个long型整数表示类型
 * @param msgsz 	消息长度，不包括type的长度
 * @param msgflg 	取值和意义如下：
 * 					IPC_NOWAIT：如果队列已满，不写入，直接返回-1
 * 					0：与IPC_NOWAIT相反
 * @return int 		成功返回0，否则返回-1
 */
int kern_msgsnd(int msqid, const void *msgp, int msgsz, int msgflg)
{
	if (msqid >= MAX_MSQ_NUM) //检查长度
		return -1;
	if (q_list[msqid].used == 0) //队列不存在，返回-1
		return -1;
	if (q_list[msqid].info.__msg_cbytes + msgsz > q_list[msqid].info.msg_qbytes) //当前队列已满
	{
		if (msgflg & IPC_NOWAIT)
			return -1;
		else
		{
			//阻塞当前进程
			return -1;
		}
	}
	if (msgp == NULL) //检查缓冲区指针合法性
		return -1;

	long type_new = *(long *)msgp; //新消息类型
	list_item *new_msg = (list_item *)kern_kmalloc(sizeof(list_item));
	write_msg_node(msqid, new_msg, msgsz, (char *)msgp, type_new);
	insert_head_node(msqid, type_new);
	int rt_t = insert_type_list(msqid, new_msg);
	int rt_f = insert_full_list(msqid, new_msg);
	if (rt_t < 0 || rt_f < 0)
	{
		kern_kfree((u32)new_msg->msg.buf);
		kern_kfree((u32)new_msg);
		return -1;
	}
	return 0;
}

/**
 * @brief 			从特定消息队列中取出一条消息
 *
 * @param msqid 	队列ID
 * @param msgp 		放置取出消息的缓冲区
 * @param msgsz 	获取消息的长度，如果比实际消息长，只会获取实际消息部分，正常返回实际长度
 *					如果msgsz比实际消息长度小，未指定MSG_NOERROR时返回-1错误，指定了则按照msgsz截断返回获取的长度（允许为0）
 *					如果msgsz比实际消息长度大，则只会获取实际消息部分，正常返回实际长度
 * @param msgtyp 	规定取出什么类型的消息，如果指定为0，则按照顺序取出整个队列中最先入队的消息
 * @param msgflg 	标志，取值的意义分别是
 * 					IPC_NOWAIT：取不出消息不等待直接返回-1，否则阻塞当前进程
 * 					MSG_NOERROR：msgsz小于消息长度时截断消息，也算成功获取，返回获取消息的长度，否则返回-1，获取失败
 * @return int 		获取成功返回实际获取到消息的长度（当然不包括type的长度），获取失败返回-1
 */
int kern_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg)
{
	int i, get_length = 0;
	//检查id
	if (q_list[msqid].used == 0)
		return -1; //队列不存在，返回-1
	//检查type
	if (msgtyp == 0)
	{
		//判断队列是否为空
		list_item *ph = q_list[msqid].head->type_list_nxt; //第一个消息结点
		if (ph == NULL || q_list[msqid].num == 0)
		{
			if (msgflg & IPC_NOWAIT)
				return -1;
			else
			{
				// TODO：阻塞当前进程
				return -1;
			}
		}
		//写目标消息
		*(long *)msgp = ph->msg.type;
		//首先判断flag中是否包含MSG_NOERROR
		if (msgflg & MSG_NOERROR)
		{
			for (i = 0; i < ph->msgsz && i < msgsz; i++, get_length++) // i既要小于消息的实际长度，也要小于规定的长度
				((char *)msgp + sizeof(long))[i] = ph->msg.buf[i];
		}
		else
		{
			if (msgsz >= ph->msgsz)
				for (i = 0; i < ph->msgsz && i < msgsz; i++, get_length++) // i既要小于消息的实际长度，也要小于规定的长度
					((char *)msgp + sizeof(long))[i] = ph->msg.buf[i];
			else
				return -1; //未指定MSG_NOERROR，不截断，返回错误码
		}

		//删除取出的节点
		rm_msg_node(msqid, q_list[msqid].head->type_list_nxt->type_list_pre);
		//更新消息数量
		q_list[msqid].num--;
		return get_length;
	}

	if (msgtyp > 0)
	{
		//根据指定的type找到目标头结点
		list_item *head_find = q_list[msqid].head;
		while (head_find && head_find->msg.type != msgtyp)
			head_find = head_find->nxt.head_nxt;
		if (head_find) //存在指定type的头结点
		{
			//拷贝目标消息内容
			list_item *tar = head_find->type_list_nxt; //目标子队列第一个消息结点
			if (tar)
			{
				//写目标消息
				*(long *)msgp = tar->msg.type;
				//首先判断flag中是否包含MSG_NOERROR
				if (msgflg & MSG_NOERROR)
				{
					for (i = 0; i < tar->msgsz && i < msgsz; i++, get_length++) // i既要小于消息的实际长度，也要小于规定的长度
						((char *)msgp + sizeof(long))[i] = tar->msg.buf[i];
				}
				else
				{
					if (msgsz >= tar->msgsz)
						for (i = 0; i < tar->msgsz && i < msgsz; i++, get_length++) // i既要小于消息的实际长度，也要小于规定的长度
							((char *)msgp + sizeof(long))[i] = tar->msg.buf[i];
					else
						return -1; //未指定MSG_NOERROR，不截断，返回错误码
				}
				//删除目标消息结点
				rm_msg_node(msqid, head_find);
				return get_length;
			}
		}
		if (msgflg & IPC_NOWAIT)
			return -1;
		else
		{
			//阻塞当前进程
			return -1;
		}
	}
	// type小于0的取出方法暂时没有实现
	return -1;
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
 * @param buf 		存放队列信息结构的指针，buf应该是msqid_ds类型，这个类型还没有定义
 * @return int 		对前三个cmd值，操作成功返回0，出错返回-1
 */
int kern_msgctl(int msqid, int cmd, msqid_ds *buf)
{
	int ret = -1;
	if (msqid < 0 || msqid > MAX_MSQ_NUM || !q_list[msqid].used) //检查队列是否存在
		return ret;
	switch (cmd)
	{
	case IPC_RMID:
	{
		list_item *i_head = q_list[msqid].head->nxt.head_nxt;
		if (q_list[msqid].info.msg_qnum > 0)
		{
			while (i_head)
			{
				while (i_head->type_list_nxt)
					rm_msg_node(msqid, i_head);
				i_head = i_head->nxt.head_nxt;
			}
		}
		i_head = q_list[msqid].head;
		while (i_head)
		{
			list_item *temp_nxt = i_head->nxt.head_nxt;
			kern_kfree((u32)i_head);
			i_head = temp_nxt;
		}
		q_list[msqid].used = 0;
		q_list[msqid].head = NULL;
		ret = 0;
		break;
	}
	case IPC_STAT:
	{
		*(msqid_ds *)buf = q_list[msqid].info; //复制信息
		ret = 0;
		break;
	}
	case IPC_SET:
		//复制信息
		q_list[msqid].info.msg_qbytes = (*(msqid_ds *)buf).msg_qbytes;
		q_list[msqid].info.msg_ctime = sys_get_ticks();
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

// utils:
/**
 * @brief 			从队列中取出删除一个消息，同时修改队列的信息
 *
 * @param msqid 	队列ID
 * @param head 		头结点指针
 */
void rm_msg_node(int msqid, list_item *head)
{
	if (!head) //输入指针为NULL，直接返回
		return;
	list_item *target = head->type_list_nxt;
	if (!target) // haed后面没有任何消息结点
		return;
	int size = target->msgsz; //要删除的消息的长度
	list_item *f_nxt = target->nxt.full_list_nxt;
	list_item *f_pre = target->pre.full_list_pre;
	list_item *t_nxt = target->type_list_nxt;
	head->type_list_nxt = t_nxt;
	if (t_nxt)
		t_nxt->type_list_pre = head;
	if (f_nxt)
		f_nxt->pre.full_list_pre = f_pre;
	if (f_pre == q_list[msqid].head)
		q_list[msqid].head->type_list_nxt = f_nxt;
	else
		f_pre->nxt.full_list_nxt = f_nxt;
	//释放target内存
	kern_kfree((u32)target->msg.buf);
	kern_kfree((u32)target);
	//修改队列信息
	q_list[msqid].info.msg_lrpid = kern_get_pid();
	q_list[msqid].info.msg_qnum--;
	q_list[msqid].info.__msg_cbytes -= size;
	q_list[msqid].info.msg_rtime = kern_get_ticks();
	return;
}

/**
 * @brief 			添加消息时，将消息内容写入队列中的结点，并修改队列的一些信息
 *
 * @param msqid 	队列ID
 * @param node 		结点指针，应分配空间后再调用此函数
 * @param msgsz 	消息长度，由用户提供
 * @param msgp 		用户提供的消息缓冲区
 * @param type_new 	新消息的类型（这个参数实际上是多余的，但由于之前老代码的局限性，还是不改为妙）
 */
void write_msg_node(int msqid, list_item *node, int msgsz, char *msgp, long type_new)
{
	int i;
	q_list[msqid].num++;
	q_list[msqid].info.msg_qnum++;
	q_list[msqid].info.__msg_cbytes += msgsz;
	q_list[msqid].info.msg_lspid = kern_get_pid();
	q_list[msqid].info.msg_stime = kern_get_ticks();
	node->msg.type = type_new;
	node->msg.buf = (char *)kern_kmalloc(msgsz);
	node->msgsz = msgsz;
	for (i = 0; i < msgsz; i++)
		node->msg.buf[i] = ((char *)msgp + sizeof(long))[i];
	return;
}

/**
 * @brief 			初始化q_list的第id个队列，键值设为key
 *
 * @param id 		q_list的位置，调用前应保证第id个队列是空闲的
 * @param key 		键值，初始化的队列键值会被设为key
 * @return int 		成功返回0，否则返回-1
 */
int newque(int id, key_t key)
{
	if (id < 0 || id >= MAX_MSQ_NUM)
		return -1;
	memset(&q_list[id], 0, sizeof(msg_queue));
	list_item *ph = (list_item *)kern_kmalloc(sizeof(list_item));
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
int ipc_findkey(key_t key)
{
	int id;
	for (id = 0; id < MAX_MSQ_NUM; id++)
	{
		if (q_list[id].key == key && q_list[id].used)
			return id;
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
list_item *insert_head_node(int msqid, long type_new)
{
	//创建新的空头结点
	list_item *new_head = (list_item *)kern_kmalloc(sizeof(list_item));
	memset(new_head, 0, sizeof(list_item));
	new_head->msg.type = type_new;
	//检查该类型是否存在
	list_item *i = NULL;
	for (i = q_list[msqid].head; i && i->msg.type <= type_new; i = i->nxt.head_nxt)
	{
		if (i->msg.type == type_new)
		{
			kern_kfree((u32)new_head);
			return NULL;
		}
	}
	if (i)
	{
		list_item *pre = i->pre.head_pre;
		pre->nxt.head_nxt = new_head;
		new_head->pre.head_pre = pre;
		new_head->nxt.head_nxt = i;
		i->pre.head_pre = new_head;
	}
	else
	{
		for (i = q_list[msqid].head; i->nxt.head_nxt; i = i->nxt.head_nxt)
			;
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
int insert_type_list(int msqid, list_item *new_msg)
{
	if (new_msg->msg.type <= 0)
		return -1;
	list_item *head = NULL;
	for (head = q_list[msqid].head; head; head = head->nxt.head_nxt)
	{
		if (head->msg.type == new_msg->msg.type)
			break;
	}
	if (head)
	{
		list_item *t = head;
		while (t->type_list_nxt)
			t = t->type_list_nxt;
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
int insert_full_list(int msqid, list_item *new_msg)
{
	if (new_msg->msg.type <= 0)
		return -1;
	list_item *i = NULL;
	for (i = q_list[msqid].head->type_list_nxt; i && i->nxt.full_list_nxt; i = i->nxt.full_list_nxt)
		;
	if (!i)
	{
		q_list[msqid].head->type_list_nxt = new_msg;
		new_msg->pre.full_list_pre = q_list[msqid].head;
	}
	else
	{
		i->nxt.full_list_nxt = new_msg;
		new_msg->pre.full_list_pre = i;
	}
	new_msg->nxt.full_list_nxt = NULL;
	return 0;
}

// void display_queue(int msqid)
// {
// 	list_item *head = q_list[msqid].head->nxt.head_nxt;
// 	printf("by type:\n");
// 	while (head)
// 	{
// 		printf("type:[%d] ", head->msg.type);
// 		list_item *i = head->type_list_nxt;
// 		while (i)
// 		{
// 			printf("%d ", i->msg.type);
// 			i = i->type_list_nxt;
// 		}
// 		printf("\n");
// 		head = head->nxt.head_nxt;
// 	}

// 	printf("by full:\n");
// 	list_item *it = q_list[msqid].head->type_list_nxt;
// 	while (it)
// 	{
// 		printf("%d ", it->msg.type);
// 		it = it->nxt.full_list_nxt;
// 	}
// 	printf("\n");
// 	return;
// }
