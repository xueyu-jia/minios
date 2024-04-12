#include "stdio.h"
#include "syscall.h"

int main(int argc, char** argv) {
    u32 free_mem = total_mem_size();
	printf("free_mem:%x\n",free_mem);
	return 0;
}