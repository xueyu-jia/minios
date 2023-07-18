#include "../include/stdio.h"

void main(int argc,char *argv[])
{
 	createdir("test");
	createdir("test/dir");
	int fd = open("test/dir/file",O_CREAT|O_RDWR);
    printf("fd = %d\n",fd);
	char buff[16] = {'a','b','c'};
	if(fd>=0){
		write(fd,buff,8);
		close(fd);
	}
	fd = open("test/dir/file",O_RDWR);
	//int ret = deletedir("test");
	memset(buff,0,8);
	read(fd,buff,8);
	printf("file content:\n");
	printf(buff);
	printf("\n");
	close(fd);
    //测试目录下有文件时的删除目录操作
	int ret = deletedir("test/dir");
    printf("delete dir when dir is not empty; ret =  %d\n",ret);
    //删除dir下的文件
	unlink("test/dir/file");
    ret = deletedir("test/dir");
    printf("delete dir when dir is  empty; ret =  %d\n",ret);
	exit(0);
	return 0;


}
