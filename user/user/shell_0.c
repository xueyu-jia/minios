//#include "stdio.h"
#include "../include/stdio.h"	//modified by mingxuan 2021-8-7

int global=1;   //added by mingxuan 2020-12-21

void main(int arg,char *argv[])
{
    /*
	int stdin = open("dev_tty0",O_RDWR);
	int stdout= open("dev_tty0",O_RDWR);
	int stderr= open("dev_tty0",O_RDWR);
	*/

    char buf[1024];
    int pid;
    int times = 0;
    
    //get_pid();
  	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father

                int exit_status;
                //exit_status = wait_();
                wait_(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child
				if(execve(buf, NULL, NULL)!=0)
				{
					printf("exec failed: file not found!");
                	exit(-1);
            	}
			}	
   		}
	}

	return ;

}