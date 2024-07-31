#include "stdio.h"

int main(int argc,char *argv[])
{
	printf("I'm test.\n");
	printf("argc:");
	printf("%d",argc);
	printf("\n");

	int i;
	for(i = 0;i < argc;i++)
	{
		printf(argv[i]);
		printf("\n");
	}
	// while(1);
	return 0;
}
