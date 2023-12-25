#include "../include/stdio.h"

void main(int arg,char *argv[])
{
    char* exec_argv[6] = {"echo","hello","world","xyx","wjh",NULL};
	printf("execvp->test:\n");
	if(execv("test_7.bin", exec_argv)==-1)
    {
        printf("execv error");
    }
    // while(1);
}