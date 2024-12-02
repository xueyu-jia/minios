#include <pthread.h>
#include <stdio.h>
int global = 0;

pthread_t ntid;

void *pthread_test1(void *args) {
  int i;
  // pthread(pthread_test2);
  // while(1)
  // {
  // 	printf("pth");
  // 	// printf("%d",++global);
  // 	printf(" ");
  // 	printf("%d %d\n",*( int *)args, *((int *)args+1));   // add
  // 	i=10000000;
  // 	while(--i){}
  // }

  int vebp = 0;
  __asm__ __volatile__("mov %%ebp,(%%eax)\n\t" : : "a"(&vebp) : "memory");
  printf("%x\n", vebp);
  // while(1)
  // {
  // printf("%d %d\n",*(int*)args,*(((int*)args)+1));
  i = 10000000;
  while (--i) {
  }
  // }
}

/*======================================================================*
                          Syscall Pthread TestS
added by xw, 18/4/27
*======================================================================*/

int main(int arg, char *argv[]) {
  /*
  void *buf = NULL;
  buf = (void*)malloc(0x80000);
  if(buf==NULL)
          printf("wrong!");
  */
  pthread_attr_t true_attr;

  int i = 0;
  int num[2] = {2021, 2022};

  true_attr.detachstate = PTHREAD_CREATE_DETACHED;
  true_attr.schedpolicy = SCHED_FIFO;
  /*true_attr->schedparam		= ??? */
  true_attr.inheritsched = PTHREAD_INHERIT_SCHED;
  true_attr.scope = PTHREAD_SCOPE_PROCESS;
  true_attr.guardsize = 0;
  true_attr.stackaddr_set = 0;
  true_attr.stackaddr = 0xd000000;
  true_attr.stacksize = 0x4000;

  // pthread_create(pthread_test1);
  // pthread_create(&ntid, NULL, pthread_test1, NULL);
  // pthread_create(&ntid, NULL, pthread_test1, &num);
  pthread_create(&ntid, &true_attr, pthread_test1, num);

  printf("ntid=%d\n", ntid);

  // while(1)
  // {
  // printf("init");
  // printf("%d",++global);
  // printf(" ");
  i = 10000000;
  while (--i) {
  }
  // }
  return 0;
}
