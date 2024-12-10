#include <msg.h>
#include <stdio.h>
#include <string.h>

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
