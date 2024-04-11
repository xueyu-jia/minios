#include "../include/stdio.h"

// int global = 1;
// char str[12];

// void main(int arg,char *argv[])
// {


// 	//printf("%s\n",str);		//deleted by mingxuan 2021-3-17
// 	printf("%s\n","abcd");
// 	printf("%d\n", global);	//deleted by mingxuan 2021-3-17
	
// 	//malloc(1024);

// 	if(0==mount("dev_sdb", "fat1", NULL, NULL, NULL))
// 	{
// 		printf("mount sdb to fat1\n");
// 	}

// 	printf(" %d ",get_pid());
// 	exit(get_pid());
// }
int main()
{
	if(fork()!=0){
		for(int i = 0; i < 10; i++){
			printf("parent:%d\n", i);
		}
		int childStatues;
		wait_(&childStatues);
		exit(0);
		return 0;
	}
	int ret;
	ret = execve("fstest.bin", NULL, NULL);
	if(ret != 0)	printf("execv error in userprograme\n");
	exit(0);
	return 0;
}
