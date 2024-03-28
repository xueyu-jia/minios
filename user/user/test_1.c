//#include "stdio.h"
#include "../include/stdio.h"	//modified by mingxuan 2021-8-29

int global = 1; //added by mingxuan 2020-12-21

/*======================================================================*
                          Syscall Pthread Test
added by xw, 18/4/27
 *======================================================================*/
/*
void pthread_test()
{
	int i;

	printf("Child thread: %d\n", get_pid());

	printf("total_mem_size before malloc:%x\n", total_mem_size());

	malloc(10); //bug is here, mingxuan 2021-8-17

	printf("total_mem_size after malloc:%x\n", total_mem_size());

	while (1)
	{
	}
}

int main(int arg, char *argv[])
{
	int i = 0;
	printf("process total_mem_size before malloc:%x\n", total_mem_size());
	malloc(4000);
	printf("total_mem_size before malloc:%x\n", total_mem_size());
	malloc(4001);

	printf("total_mem_size before malloc:%x\n", total_mem_size());

	pthread(pthread_test);

	printf("Father process: %d\n", get_pid());

	while (1)
	{
	}

	return 0;
}
*/

/*======================================================================*
                    File System Test-测试fat32
added by mingxuan 2019-5-18
 *======================================================================*/
#define BUF_SIZE 4096		//4KB
//#define BUF_SIZE 8196		//8KB
//#define BUF_SIZE 16384	//16KB
//#define BUF_SIZE 32768	//32KB
//#define BUF_SIZE 65536	//64KB

char bufw[BUF_SIZE];
char bufr[BUF_SIZE];

void main(int arg,char *argv[])
{
	// if(fork() != 0){
	// 	while(1){};
	// 	for(unsigned int i = 0; i < 0xFFFFFFF0; i++){
	// 		for(unsigned int j = 0; i < 0xFFFFFFF0; i++){
	// 			for(unsigned int k = 0; i < 0xFFFFFFF0; i++){
	// 			}
	// 		}

	// 	}
	// }
	int fd;
	char filename[] = "fat0/test33.txt";

	//char bufw[4096];	//the largest size is 16KB, beacause the stack size is 16KB
	//char bufr[4096];

	int i=0;
	for(i=0;i<BUF_SIZE-1;i++)
	{
		bufw[i]='a';
	}
	bufw[BUF_SIZE-1]='\0';

	//测试写文件的大小
	fd = open(filename, O_CREAT | O_RDWR);
	if(fd != -1)
	{
		write(fd, bufw, strlen(bufw));
		close(fd);
	}

	//测试读文件的大小
	//先读
	fd = open("fat0/test33.txt", O_RDWR);
	if(fd != -1)
	{
		read(fd, bufr, BUF_SIZE);
		//udisp_str(bufr);
		close(fd);
	}
	//把读到的内容写到新文件中，验证是否读正确
	fd = open("fat0/test44.txt", O_CREAT | O_RDWR);
	if(fd != -1)
	{
		write(fd, bufr, strlen(bufr));
		close(fd);
	}

	exit(0);

	return;
}
