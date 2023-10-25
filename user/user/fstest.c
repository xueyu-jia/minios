#include "../include/stdio.h"
/*
 * 长路径
 * 测试系统调用 
 * 创建、删除目录
 * 创建、删除文件
 * 文件读写
 * 打开文件、关闭文件
 * 
*/
void main(int argc,char *argv[])
{
 	createdir("/fs_len_test");
	createdir("/fs_len_test/dir");
	int fd = open("/fs_len_test/dir/file.txt",O_CREAT|O_RDWR);
    printf("fd = %d\n",fd);
	char buff[16] = {'a','b','c'};
	if(fd>=0){
		write(fd,buff,8);
		close(fd);
	}
	fd = open("/fs_len_test/dir/file.txt",O_RDWR);
	//int ret = deletedir("test");
	memset(buff,0,8);
	read(fd,buff,8);
	printf("file content:\n");
	printf(buff);
	printf("\n");
	close(fd);
    //测试目录下有文件时的删除目录操作
	int ret = deletedir("/fs_len_test/dir");
    printf("delete dir when dir is not empty; ret =  %d\n",ret);
    //删除dir下的文件
	unlink("/fs_len_test/dir/file.txt");
    ret = deletedir("/fs_len_test/dir");
    printf("delete dir when dir is empty; ret =  %d\n",ret);


	char path[128];
	memset(path, 0, 128);
	int i;
	for(i = 0; i < 122; i++){
		path[i++] = '/'; 
		path[i++] = 'a' + (i % 26);
		path[i++] = 'a' + (i % 26);
		path[i++] = 'a' + (i % 26);
		path[i++] = 'a' + (i % 26);
		path[i++] = 'a' + (i % 26);
		path[i] = 'a' + (i % 26);
		createdir(path);
	}
	printf("path:%s\n", path);
	printf("path length :%d\n", strlen(path));

	ret = deletedir("error");
	printf("delete error path ret:%d\n", ret);
	ret = deletedir(path);
	printf("long path ret:%d\n", ret);
	exit(0);
	return 0;


}
