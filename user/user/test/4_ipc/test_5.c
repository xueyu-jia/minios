// #include "../include/stdio.h"
// #include "../include/msg.h"

// typedef struct my_msg{
// 	long t;
// 	char m[20];
// }my_msg;

// void clear_msg(my_msg* m){
// 	int i;
// 	for(i=0;i<20;i++){
// 		m->m[i] = '\0';
// 	}
// }

// int main(int arg, char *argv[])
// {
// 	printf("%d\n", -1);
// 	key_t key_0 = ftok("string bvhew", 999);
// 	key_t key_1 = ftok("string wettyu", 100);
// 	int q1 = msgget(key_0, IPC_CREAT|IPC_EXCL);
// 	int q2 = msgget(key_1, IPC_CREAT|IPC_EXCL);
// 	if(q1<0 || q2<0){
// 		printf("queue exist.\n");
// 		exit(1);
// 	}

// 	my_msg M = {1, "MESSAGE"}, recv;
// 	clear_msg(&recv);
// 	msgsnd(q1, &M, 3, IPC_NOWAIT);
// 	msgsnd(q2, &M, 7, IPC_NOWAIT);

// 	int l;
// 	l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
// 	printf("from q1 length: %d, type: %d, msg:%s\n", l, recv.t, recv.m);
// 	clear_msg(&recv);
// 	l = msgrcv(q2, &recv, 2, 0, IPC_NOWAIT|MSG_NOERROR);
// 	printf("from q2 length: %d, type: %d, msg:%s\n", l, recv.t, recv.m);

// 	l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
// 	printf("rt:%d\n", l);

// 	printf("read done\n");

// 	int p_child = fork();
// 	if(p_child == 0){
// 		//child
// 		printf("child\n");
// 		int q1_c = msgget(key_0, IPC_CREAT);
// 		M.t = 999;
// 		msgsnd(q1_c, &M, 1, IPC_NOWAIT);
// 		exit(0);
// 	}else{
// 		printf("parent\n");
// 		wait();
// 		clear_msg(&recv);
// 		l = msgrcv(q1, &recv, 10, 0, IPC_NOWAIT|MSG_NOERROR);
// 		printf("(parent)from q1 length: %d, type: %d, msg:%s\n", l,
// recv.t, recv.m);
// 	}

// 	msgctl(q1, IPC_RMID, NULL);
// 	msgctl(q2, IPC_RMID, NULL);

// 	exit(0);
// 	return 0;
// }

#include "../include/msg.h"
#include "../include/stdio.h"

typedef struct my_msg {
  long t;
  char m[20];
} my_msg;

void clear_msg(my_msg* m) {
  int i;
  for (i = 0; i < 20; i++) {
    m->m[i] = '\0';
  }
}

int main(int arg, char* argv[]) {
  printf(" *** multi-queue *** \n");
  int q1, q2, q3;
  my_msg m1 = {1, "send from parent"}, m2 = {2, "send from child 1"},
         m3 = {3, "sned from child 2"};

  key_t key1 = ftok("qwer", 1);
  key_t key2 = ftok("asdf", 2);
  key_t key3 = ftok("zxcv", 3);

  q1 = msgget(key1, IPC_CREAT | IPC_EXCL);
  if (q1 < 0) {
    printf("q1 create error\n");
    exit(0);
  }
  msgsnd(q1, &m1, 16, IPC_NOWAIT);

  int pid1 = fork();
  if (pid1 == 0) {  // child 1
    q2 = msgget(key2, IPC_CREAT | IPC_EXCL);
    if (q2 < 0) {
      printf("q2 create error\n");
    } else {
      msgsnd(q2, &m2, 17, IPC_NOWAIT);
    }
    exit(0);
  } else {
    wait(NULL);
  }

  int pid2 = fork();
  if (pid2 == 0) {  // child 2
    q3 = msgget(key3, IPC_CREAT | IPC_EXCL);
    if (q3 < 0) {
      printf("q3 create error\n");
    } else {
      msgsnd(q3, &m3, 17, IPC_NOWAIT);
      // receive from q1
      my_msg recv;
      memset(recv.m, 0, 20);
      msgrcv(q1, &recv, 20, 0, IPC_NOWAIT | MSG_NOERROR);
      printf("(child2)type:%d, message:%s\n", recv.t, recv.m);
    }
    exit(0);
  } else {
    wait(NULL);
  }

  q2 = msgget(key2, 0);
  q3 = msgget(key3, 0);

  my_msg recv;
  memset(recv.m, 0, 20);
  msgrcv(q2, &recv, 20, 0, IPC_NOWAIT | MSG_NOERROR);
  printf("(parent)q2: type:%d, message:%s\n", recv.t, recv.m);

  memset(recv.m, 0, 20);
  msgrcv(q3, &recv, 20, 0, IPC_NOWAIT | MSG_NOERROR);
  printf("(parent)q3: type:%d, message:%s\n", recv.t, recv.m);

  exit(0);
  return 0;
}
