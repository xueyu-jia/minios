#include "stdio.h"

int global=1;   //added by mingxuan 2020-12-21

void main(int arg,char *argv[])
{
  
    printf("%d", get_pid());
    udisp_int(get_pid());

	// while(1);

	return ;

}