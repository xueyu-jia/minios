#include "../include/stdio.h"
#include "../include/msg.h"

typedef struct my_msg{
	long t;
	char m[20];
}my_msg;

void clear_msg(my_msg* m){
	int i;
	for(i=0;i<20;i++){
		m->m[i] = '\0';
	}
}

int main(int arg, char *argv[])
{
	printf("%d\n", -1);
	key_t key_0 = ftok("string bvhew", 999);
	key_t key_1 = ftok("string wettyu", 100);
	int q1 = msgget(key_0, IPC_CREAT|IPC_EXCL);
	int q2 = msgget(key_1, IPC_CREAT|IPC_EXCL);
	if(q1<0 || q2<0){
		printf("queue exist.\n");
		exit(1);
	}

	my_msg M = {1, "MESSAGE"}, recv;
	clear_msg(&recv);
	msgsnd(q1, &M, 3, IPC_NOWAIT);
	msgsnd(q2, &M, 7, IPC_NOWAIT);

	int l;
	l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
	printf("from q1 length: %d, type: %d, msg:%s\n", l, recv.t, recv.m);
	clear_msg(&recv);
	l = msgrcv(q2, &recv, 2, 0, IPC_NOWAIT|MSG_NOERROR);
	printf("from q2 length: %d, type: %d, msg:%s\n", l, recv.t, recv.m);

	l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
	printf("rt:%d\n", l);

	printf("read done\n");

	int p_child = fork();
	if(p_child == 0){
		//child
		printf("child\n");
		int q1_c = msgget(key_0, IPC_CREAT);
		M.t = 999;
		msgsnd(q1_c, &M, 1, IPC_NOWAIT);
		exit(0);
	}else{
		printf("parent\n");
		wait_();
		clear_msg(&recv);
		l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
		printf("(parent)from q1 length: %d, type: %d, msg:%s\n", l, recv.t, recv.m);
	}

	msgctl(q1, IPC_RMID, NULL);
	msgctl(q2, IPC_RMID, NULL);
	
	exit(0);
	return 0;
}