#include "../include/stdio.h"

int main(int arg,char *argv[])
{
    char* exec_argv[6] = {"echo","hello","world","xyx","wjh",NULL};
	printf("execvp->test:\n");
	if(execv("test_7.bin", exec_argv)==(u32)-1)
    {
        printf("execv error");
    }
    // while(1);
	return 0;
}