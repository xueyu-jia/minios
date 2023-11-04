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
	// 创建长目录
	char path[128], buff[128], filename[128];
	int ret, fd;
	memset(path, 0, 128);
	memset(filename, 0, 128);
	memset(buff, 0, 128);
	int i;
	for(i = 0; i < 118; i++){
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
	// 在长目录下创建文件并写入数据
	strcpy(filename, path);
	filename[i++] = '/';
    while(i < 127){
        filename[i++] = 'A' + (i % 26);
    }
	filename[i] = 0;
	printf("path + filename length :%d\n", strlen(filename));
	fd = open(filename, O_CREAT|O_RDWR);
	if(fd >= 0){
		write(fd, filename, 128);
		close(fd);
	}else{
		printf("open file error/n");
		exit(0);
		return 0;
	}

	// 打开文件并读入数据
	fd = open(filename, O_RDWR);
	read(fd, buff, 128);
	close(fd);
	printf("file content: \n%s\n", buff);
	printf("content length :%d\n", strlen(buff));

	// 删除不存在的目录
	ret = deletedir("error");
	printf("delete error path ret:%d\n", ret);

	// 删除有文件的目录
	ret = deletedir(path);
	for(int j = 0; j < 100000000; j++);
	printf("delete 1 ret:%d\n", ret);
	
	// 删除文件,然后再删除目录
	ret = unlink(filename);
	for(int j = 0; j < 100000000; j++);
	printf("delete file ret:%d\n", ret);
	
	ret = deletedir(path);
	for(int j = 0; j < 100000000; j++);
	printf("delete 2 ret:%d\n", ret);
	
	exit(0);
	return 0;


}
