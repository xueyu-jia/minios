//#include "stdio.h"
#include "../include/stdio.h"	//modified by mingxuan 2021-4-5
#include "../include/malloc.h"	//added by mingxuan 2021-4-5

int global=0;

char *str2,*str3;


void pthread_test3()
{
	int i;
	str2 = malloc(10);
	*(str2+0) = 'M';
	*(str2+1) = 'a';
	*(str2+2) = '\0';
	
	while(1)
	{
		if(str3!=0)
		{
			udisp_str("pth3");
			(*(str2+1)) += 1;
			udisp_str(str3);
			udisp_str(" ");
		}		
		i=10000;
		while(--i){}
	}
}


void pthread_test2()
{
	int i;
	str3 = malloc(10);
	*(str3+0) = 'M';
	*(str3+1) = 'z';
	*(str3+2) = '\0';
	
	// pthread_create(pthread_test3);	
	while(1)
	{
		if(str2!=0)
		{
			udisp_str("pth2");
			(*(str3+1)) -=1;
			udisp_str(str2);
			udisp_str(" ");
		}
		
		i=10000;
		while(--i){}
	}
}

void pthread_test1()
{
	int i;
	// pthread_create(pthread_test2);
	while(1)
	{
		udisp_str("pth1");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
}

/*======================================================================*
                          Syscall Pthread Test
added by xw, 18/4/27
 *======================================================================*/
/*
int main(int arg,char *argv[])
{

	int i=0;
	
	pthread(pthread_test1);
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return 0;
}
*/

/*======================================================================*
                          Syscall Fork Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	fork();
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           Syscall Exec Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           Syscall Yield Test
added by xw, 18/8/16
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("U( ");
		yield();
		udisp_str(") ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           Syscall Sleep Test
added by xw, 18/8/16
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("U( ");
		udisp_str("[");
		udisp_int(get_ticks());
		udisp_str("] ");
		sleep(5);
		udisp_str("[");
		udisp_int(get_ticks());
		udisp_str("] ");
		udisp_str(") ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           File System Test-测试orange
added by xw, 18/6/19
 *======================================================================*/
/*
void main(int arg,char *argv[])
{

	int fd;
	int i, n;
	const int rd_bytes = 4;
	//char filename[MAX_FILENAME_LEN+1] = "blah";
	char filename[MAX_FILENAME_LEN+1] = "orange/blah";
	char bufr[5];
	const char bufw[] = "abcde";

	udisp_str("\n(U)");

	fd = open(filename, O_CREAT | O_RDWR);	
	
	if(fd != -1) {
		udisp_str("File created: ");
		udisp_str(filename);
		udisp_str(" (fd ");
		udisp_int(fd);
		udisp_str(")\n");	
		
		n = write(fd, bufw, strlen(bufw));
		if(n != strlen(bufw)) {
			udisp_str("Write error\n");
		}
		
		close(fd);
	}
	
	udisp_str("(U)");
	fd = open(filename, O_RDWR);
	udisp_str("   ");
	udisp_str("File opened. fd: ");
	udisp_int(fd);
	udisp_str("\n");

	udisp_str("(U)");
	int lseek_status = lseek(fd, 1, SEEK_SET);
	udisp_str("Return value of lseek is: ");
	udisp_int(lseek_status);
	udisp_str("  \n");

	udisp_str("(U)");
	n = read(fd, bufr, rd_bytes);
	if(n != rd_bytes) {
		udisp_str("Read error\n");
	}
	bufr[n] = 0;
	udisp_str("Bytes read: ");
	udisp_str(bufr);
	udisp_str("\n");

	close(fd);
	

	while (1) {
	}
	
	return;
}
*/

/*======================================================================*
                    File System Test-测试fat32
added by mingxuan 2019-5-18
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	
	int fd;
	char filename[] = "fat0/test33.txt";
	//char filename[] = "orange/test34.txt";
	//char bufw[] = "abcd23";
	
	char bufw[10];
	//char bufw[2048]; //4个扇区
	//char bufw[512];
	char bufr[256];

	int i=0;
	for(i=0;i<9;i++)
	{
		bufw[i]='a';
	}
	//bufw[2045]='q';
	//bufw[2046]='l';
	bufw[9]='\0';

	//create(filename);
	fd = open(filename, O_CREAT | O_RDWR);
	//fd = open(filename, O_RDWR);
	if(fd != -1) 
	{
		write(fd, bufw, strlen(bufw));
		close(fd);
	}

	fd = open(filename, O_RDWR);
	if(fd != -1)
	{
		read(fd, bufr, 256);
		udisp_str(bufr);
		close(fd);
	}

	
	while (1) {
	}
	
	
	return;
}
*/

// added by rzr, pgw, 2020

#include "util.h"

#define O_CREAT 1
#define O_RDWR 2
#define PATH_DEL '\\'

char workdev[16];
char workdir[256];

//deleted by mingxuan 2021-8-7

int argc;
char argv[8][256];

extern int tty;

void parse_args(char *rbuf)
{
	argc = 0;
	char *iptr = rbuf;
	char *optr = argv[argc];
	while (*iptr != '\n')
	{
		if (*iptr != ' ')
		{
			*optr++ = *iptr++;
			continue;
		}
		*optr = 0;
		++iptr;
		optr = argv[++argc];
	}
	*optr = 0;
	++argc;
}


char* strchrs(char *s1, char *s2)
{
	while (*s1)
	{
		char *r = s2;
		while (*r)
		{
			if (*s1 == *r)
			{
				return s1;
			}
			++r;
		}
		++s1;
	}
	return 0;
}

void builtin_cat()
{
	if (argc == 1)
	{
		write(tty, "cat: cat [file]\n", 17);
		return;
	}
	int total;
	int len;
	char fullpath[256];
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, workdir);
	len = strlen(fullpath);
	if (fullpath[len - 1] != PATH_DEL)
	{
		fullpath[len++] = PATH_DEL;
	}
	strcpy(fullpath + len, argv[1]);
	char buf[512];
	int fd = fat_open(fullpath, O_RDWR);
	if (fd <= 0)
	{
		write(tty, "ERROR: file not exists\n", 24);
		return;
	}
	total = 0;
	len = read(fd, buf, 512);
	while (len > 0)
	{
		total += len;
		write(tty, buf, len);
		len = read(fd, buf, 512);
	}
	fprintf(tty, "cat: read %d bytes\n", total);
	close(fd);
}

void buildin_chdev()
{
	if (argc == 1)
	{
		fprintf(tty, "chdev: chdev [dev]\n");
		return;
	}
	char olddev[16];
	strcpy(olddev, workdev);
	strcpy(workdev, argv[1]);
	if (!fat_chdir("V:\\"))
	{
		strcpy(workdev, olddev);
		fprintf(tty, "ERROR: dev not exists\n");
		return;
	}
	strcpy(workdir, "\\");
}

void builtin_chdir()
{
	if (argc == 1)
	{
		write(tty, "cd: cd [dir]\n", 14);
		return;
	}
	if (!strcmp(argv[1], "."))
	{
		return;
	}
	char pathname[256];
	char fullpath[256];
	char wbuf[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (!strcmp(argv[1], ".."))
	{
		char *trunc = strrchr(pathname + 1, PATH_DEL);
		if (trunc)
		{
			*trunc = 0;
		}
		else
		{
			pathname[1] = 0;
		}
		strcpy(fullpath, "V:");
		strcpy(fullpath + 2, pathname);
		//opendir(fullpath);
		fat_chdir(fullpath);
		strcpy(workdir, pathname);
		return;
	}
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, pathname);
	//if (opendir(fullpath) != 1)
	if (fat_chdir(fullpath) != 1)
	{
		write(tty, "ERROR: no such directory\n", 26);
		return;
	}
	strcpy(workdir, pathname);
}

void builtin_lsdir()
{
	char fullpath[256];
	int len = strlen(workdev);
	strcpy(fullpath, workdev);
	strcpy(fullpath + len, "V:");
	len += 2;
	strcpy(fullpath + len, workdir);
	listdir(fullpath);
}

void builtin_find()
{
    if (argc == 1)
    {
        fprintf(tty, "find: find [filename]\n");
        return;
    }
    findfile(argv[1]);
}

void builtin_mkdir()
{
	if (argc == 1)
	{
		write(tty, "mkdir: mkdir [dir]\n", 14);
		return;
	}
	char pathname[256];
	char fullpath[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (strchrs(argv[1], "<>:,*?/\\"))
	{
		write(tty, "ERROR: not a valid name\n", 25);
		return;
	}
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, pathname);
	int state = fat_createdir(fullpath);
	if (state != 1)
	{
		write(tty, "ERROR: directory exists\n", 25);
	}
	//fat_opendir(workdir);
}

void builtin_pwd()
{
	char wbuf[256];
	getcwd(wbuf, 256);
	strcpy(wbuf, wbuf + 2);
	int len = strlen(wbuf);
	wbuf[len++] = '\n';
	write(tty, wbuf, len);
}

void builtin_rm()
{
	if (argc == 1)
	{
		write(tty, "rm: rm [file]\n", 14);
		return;
	}
	char pathname[256];
	char fullpath[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, pathname);
	int state = fat_delete(fullpath);
	if (state != 1)
	{
		write(tty, "ERROR: file not exists\n", 23);
	}
}

void builtin_rmdir()
{
	if (argc == 1)
	{
		write(tty, "rmdir: rmdir [dir]\n", 14);
		return;
	}
	char pathname[256];
	char fullpath[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, pathname);
	int state = fat_deletedir(fullpath);
	if (state != 1)
	{
		write(tty, "ERROR: directory not exists\n", 29);
	}
}

void write_test()
{
	fprintf(tty, "FAT32 write test start\n");
	char buf[9000];
	char *optr = buf;
	int len;
	for (int i = 1; i <= 2000; i++)
	{
		len = sprintf(optr, "%d\n", i);
		optr += len;
	}
	int total;
	total = optr - buf;
	optr = buf;
	chdir("fat1/V:\\");
	createdir("fat1/b");
	chdir("fat1/b");
	int fd;
	fd = open("fat1/2000.txt", O_RDWR | O_CREAT);
	len = 512;
	while (optr < buf + total)
	{
		if (optr + len > buf + total)
		{
			len = buf + total - optr;
		}
		write(fd, optr, len);
		optr += len;
	}
	close(fd);
	chdir("fat1/V:\\");
	fprintf(tty, "write %d bytes\n", total);
	fprintf(tty, "FAT32 write test finish\n");
}

void fat32_test()
{
	fprintf(tty, "FAT32 test begin\n");
	int pid_child = fork();
	int buf[256];
	int fd;
	int i = 0;
	chdir("fat0/V:\\");
	if (!pid_child)
	{
		getcwd(buf, 256);
		fprintf(tty, "child cwd is: %s\n", buf);
		createdir("fat0/child");
		chdir("fat0/child");
		getcwd(buf, 256);
		fprintf(tty, "child cwd changes to: %s\n", buf);
		fd = open("fat0/path.txt", O_RDWR | O_CREAT);
		write(fd, buf, strlen(buf));
		close(fd);
		fprintf(tty, "child process exit\n");
		while (1);
	}
	getcwd(buf, 256);
	fprintf(tty, "parent cwd is: %s\n", buf);
	createdir("fat0/parent");
	chdir("fat0/parent");
	getcwd(buf, 256);
	fprintf(tty, "parent cwd changes to: %s\n", buf);
	fd = open("fat0/path.txt", O_RDWR | O_CREAT);
	write(fd, buf, strlen(buf));
	close(fd);
	fprintf(tty, "parent process exit\n");
	while (1);
}

void builtin_tee()
{
	if (argc == 1)
	{
		write(tty, "tee: tee [file]\n", 17);
		return;
	}
	int len;
	char fullpath[256];
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, workdir);
	len = strlen(fullpath);
	if (fullpath[len - 1] != PATH_DEL)
	{
		fullpath[len++] = PATH_DEL;
	}
	strcpy(fullpath + len, argv[1]);
	char buf[512];
	int fd = fat_open(fullpath, O_RDWR | O_CREAT);
	if (fd <= 0)
	{
		write(tty, "ERROR: file not exists\n", 24);
		return;
	}
	len = read(tty, buf, 512);
	write(fd, buf, len);
	close(fd);
}


/*======================================================================*
                    		验证多个FAT32
											added by mingxuan 2021-2-7
 *======================================================================*/
//验证多个FAT32
/*
void main()
{
	tty = open("dev_tty0", O_RDWR);
	char rbuf[256];
	strcpy(workdir, "\\");
	strcpy(workdev, "fat0/");
	//进程工作目录初始化
	fat_chdir("V:\\");
	while (1)
	{
		fprintf(tty, "miniOS:%sV:%s $ ", workdev, workdir);
		int len = read(tty, rbuf, 255);
		rbuf[len] = 0;
		parse_args(rbuf);
		if (!strcmp(argv[0], "cat"))
		{
			builtin_cat();
		}
		if (!strcmp(argv[0], "cd"))
		{
			builtin_chdir();
		}
		if (!strcmp(argv[0], "chdev"))
		{
			buildin_chdev();
		}
		if (!strcmp(argv[0], "ls"))
		{
			builtin_lsdir();
		}
		if (!strcmp(argv[0], "mkdir"))
		{
			builtin_mkdir();
		}
		if (!strcmp(argv[0], "pwd"))
		{
			builtin_pwd();
		}
		if (!strcmp(argv[0], "rm"))
		{
			builtin_rm();
		}
		if (!strcmp(argv[0], "rmdir"))
		{
			builtin_rmdir();
		}
		if (!strcmp(argv[0], "find"))
        {
		    builtin_find();
        }
		if (!strcmp(argv[0], "tee"))
		{
			builtin_tee();
		}
		if (!strcmp(argv[0], "fat32_test"))
		{
			fat32_test();
		}
		if (!strcmp(argv[0], "write_test"))
		{
			write_test();
		}
	}
}
*/

/*======================================================================*
                    		启动shell_0
											added by mingxuan 2021-2-7
 *======================================================================*/
//启动shell_0

void main(int arg,char *argv[])
{
	int stdin = open("/dev/tty0",O_RDWR);
	int stdout= open("/dev/tty0",O_RDWR);
	int stderr= open("/dev/tty0",O_RDWR);

	printf("init:toatal_mem_size=%x\n",total_mem_size());

	//char filename[30] = "fat0/shell_0.bin";
	//printf("hello world!\n");
	if(0!=fork())
	{//father
		while(1);
	}
	else
	{//child
		// execve("fat0/shell_0.bin");
// #ifdef FAT32_BOOT
// 	execve("fat0/shell_0.bin",NULL,NULL);
// #endif
// #ifdef ORANGE_BOOT
	execve("shell_0.bin",NULL,NULL);
// #endif
		//execve("fat0/shell_0.bin",NULL,NULL);

		//execve(filename);
	}
	
	return;
}





