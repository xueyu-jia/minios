#include "stdio.h"

int global=1;   //added by mingxuan 2020-12-21

void parse(char* buf, char** argv, int limit){
	if(!buf)return;
	int start = 0, cnt = 0;
	char *p = buf;
	for(; *p && *p != '\n'; p++){
		if(*p == ' ' && start == 1){
			*p = 0;
			start = 0;
		}else if(start == 0 && (*p != ' ') && cnt < limit){
			*(argv++) = p;
			start = 1;
		}
	}
	if(start)
		*p = 0;
	*argv = 0;
}

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
	char * args[4];
  	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			int pid = fork();
			if(pid!=0)
			{	//father

                int exit_status;
                wait(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
				printf("pid:%d exit_status:%d\n", pid, exit_status);
			}
			else
			{	//child
				parse(buf, args, 4);
				if(execve(buf, args, NULL)!=0)
				{
					printf("exec failed: file not found!");
                	continue;
            	}
			}	
            // if(execve(buf)!=0){
            //     printf("exec failed: file not found!");
            //     continue;
            // }
   		}
	}

	return ;

}