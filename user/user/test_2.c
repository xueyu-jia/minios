#include "stdio.h"

int global = 1;
//char *str = "abcd";
char str[12];

void main(int arg,char *argv[])
{


	//printf("%s\n",str);		//deleted by mingxuan 2021-3-17
	printf("%s\n","abcd");
	printf("%d\n", global);	//deleted by mingxuan 2021-3-17
	
	//malloc(1024);

	printf(" %d ",get_pid());
	exit(get_pid());
}
