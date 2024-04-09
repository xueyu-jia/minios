#include "stdio.h"
#include "string.h"
#define MAX_ARGC	8
#define NUM_BUILTIN_CMD	12
#define CMD_LEN	8

#define SHELL_TEST

struct cmd{
	char cmd_name[CMD_LEN];
	int (*handler)(int , char**);
};

int do_ls(int argc, char** argv){
	char* path = ".";// default
	if(argc > 2){
		printf("usage: ls [%%path]\n");
		return -1;
	}else if(argc == 2){
		path = argv[1];
	}
	DIR* dirp = opendir(path);
	if(!dirp){
		printf("opendir err\n");
		return -1;
	}
	struct dirent* ent = NULL;
	while((ent = readdir(dirp) )!= NULL){
		printf("%d %s\n", ent->d_ino, ent->d_name);
	};
	closedir(dirp);
	return 0;
}

int do_cd(int argc, char** argv){
	char* path = argv[1];
	return chdir(path);
}

int do_pwd(int argc, char** argv){
	char path[MAX_PATH];
	getcwd(path, MAX_PATH);
	printf("%s\n", path);
	return 0;
}

int do_mkdir(int argc, char** argv){
	if(argc != 2){
		printf("usage: mkdir %%path\n");
		return -1;
	}
	return mkdir(argv[1], I_RWX);
}

int do_mount(int argc, char** argv){
	if(argc != 4){
		printf("usage: mount %%dev %%target %%fstype\n");
		return -1;
	}
	return mount(argv[1], argv[2], argv[3], 0, NULL);
}

int do_date(int argc, char** argv){
	struct tm time= {0};
	get_time(&time);
	printf("%d-%02d-%02d %02d:%02d:%02d\n", 
	time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	return 0;
}

int do_write(int argc, char** argv){
	char buf[512];
	int fd = open(argv[1], O_WRONLY|O_CREAT, I_R|I_W);
	if(fd < 0) {
		printf("open error");
		return -1;
	}
	lseek(fd, 0, SEEK_END);
	while((gets(buf) && strlen(buf) != 0)) {
		write(fd, buf, strlen(buf));
	}
	close(fd);
	return 0;
}

int do_cat(int argc, char** argv){
#define MAX_CAT 128
	char buf[MAX_CAT];
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		printf("open error");
		return -1;
	}
	int cnt;
	cnt = read(fd, buf, MAX_CAT);
	write(STD_OUT, buf, cnt);
	printf("\n");
	close(fd);
	return 0;
}

int do_cat_hex(int argc, char** argv) {
#define MAX_HEX 512
#define print_hex(c) (((c) >= ' ' && (c) < 0x7F)? (c) :'.')
	char buf[MAX_HEX];
	if(argc < 4){
		printf("usage: hex %%file %%block %%offset [%%len]\n");
		return -1;
	}
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		printf("open error");
		return -1;
	}
	int cnt, len, total = MAX_HEX;
	int line_len = 16;
	char c;
	if(argc == 5) {
		total = min(MAX_HEX, atoi(argv[4]));
	}
	lseek(fd,  atoi(argv[2])*4096 + atoi(argv[3]), SEEK_SET);
	cnt = read(fd, buf, total);
	for(int i = 0; i < cnt;) {
		len = min(line_len, cnt - i);
		for(int j = 0; j < len; j++) {
			printf("%02x ", (u8)buf[i + j]);
		}
		for(int j = 0; j < len; j++) {
			printf("%c", print_hex(buf[i + j]));
		}
		i += len;
		printf("\n");
	}
	close(fd);
	return cnt;
}

int do_unlink(int argc, char** argv) {
	if(argc != 2){
		printf("usage: unlink %%path\n");
		return -1;
	}
	return unlink(argv[1]);
}

int do_rmdir(int argc, char** argv) {
	if(argc != 2){
		printf("usage: rmdir %%path\n");
		return -1;
	}
	return rmdir(argv[1]);
}

int do_chkmem(int argc, char** argv) {
	printf("mem_size=0x%x\n",total_mem_size());
	return 0;
}

struct cmd cmds[NUM_BUILTIN_CMD];
int num_cmd;

void reg_cmd(char* name, int (*handler)(int , char**)){
	if(num_cmd >= NUM_BUILTIN_CMD){
		return;
	}
	strcpy(cmds[num_cmd].cmd_name, name);
	cmds[num_cmd].handler = handler;
	num_cmd++;
}

int parse(char* buf, char** argv, int limit){
	if(!buf)return 0;
	int start = 0, cnt = 0;
	char *p = buf;
	for(; *p && *p != '\n'; p++){
		if(*p == ' ' && start == 1){
			*p = 0;
			start = 0;
		}else if(start == 0 && (*p != ' ') && cnt < limit){
			*(argv++) = p;
			cnt++;
			start = 1;
		}
	}
	if(start)
		*p = 0;
	*argv = 0;
	return cnt;
}

int match_build_in(int argc, char** argv){
	// printf("%s", argv[0]);
	for(int i = 0; i < NUM_BUILTIN_CMD; i++){
		// printf("%s\n", cmds[i].cmd_name);
		if(argc && (!strcmp(argv[0], cmds[i].cmd_name))){
			// printf("match %s\n", cmds[i].cmd_name);
			return i;
		}
	}
	return -1;
}

void printstring(char *prompt, char **p) {
    char *pstr = NULL;
    printf("%s", prompt);
    if(*p) {
        while(((pstr)=(*p++),pstr)) {
            printf("\n%s", pstr);
        }
    }
    printf("\n");
}

int main(int arg,char *argv[],char *envp[])
{
    /*
	int stdin = open("dev_tty0",O_RDWR);
	int stdout= open("dev_tty0",O_RDWR);
	int stderr= open("dev_tty0",O_RDWR);
	*/
	printf("argc:%d\n", arg);
    printstring("argv:", argv);
    printstring("env:", envp);
    char buf[1024];
	char pwd[MAX_PATH];
    int pid;
    int times = 0;
	int pre = 0;
	char * args[MAX_ARGC];
	reg_cmd("ls", do_ls);
	reg_cmd("cd", do_cd);
	reg_cmd("pwd", do_pwd);
	reg_cmd("mkdir", do_mkdir);
	reg_cmd("mount", do_mount);
	reg_cmd("date", do_date);
	reg_cmd("write", do_write);
	reg_cmd("cat", do_cat);
	reg_cmd("hex", do_cat_hex);
	reg_cmd("unlink", do_unlink);
	reg_cmd("rmdir", do_rmdir);
	reg_cmd("chkmem", do_chkmem);
	#ifdef SHELL_TEST
	#define TEST_CMD_LEN_LIMIT	32

	#define TEST_CMD_NUM 0
	char pre_test_cmds[TEST_CMD_NUM][TEST_CMD_LEN_LIMIT] = {
		"mkdir fat",
		"mount /dev/sda1 fat fat32",
		// "cd ora",
		// "/t_xv6.bin",
	};
	
	pre = TEST_CMD_NUM;
	#endif
  	while(1)
	{
        printf("\nminiOS:%s $ ",getcwd(pwd, MAX_PATH));
		#ifdef SHELL_TEST
		if(pre){
			strcpy(buf, pre_test_cmds[TEST_CMD_NUM - pre]);
			printf("%s\n", buf);
			pre--;
		}else{
			gets(buf);
		}
		#else 
			gets(buf);
		#endif
        if(strlen(buf)!=0 )
		{
			int argc = parse(buf, args, MAX_ARGC);
			// printf("%d ", argc);
			// for(int i=0;i<argc;i++){
			// 	printf("%s%c", args[i], (i==argc-1)?'\n':' ');
			// }
			int cmdid = match_build_in(argc, args);
			if(cmdid != -1){
				int ret = cmds[cmdid].handler(argc, args);
				printf("%s exit %d", cmds[cmdid].cmd_name, ret);
				continue;
			}
			int pid = fork();
			if(pid!=0)
			{	//father

                int exit_status;
                wait(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
				printf("pid:%d exit_status:%d\n", pid, exit_status);
			}
			else
			{	//child
				if(execve(buf, args, NULL)!=0)
				{
					printf("exec failed: file not found!");
                	exit(-1);
            	}
			}	
            // if(execve(buf)!=0){
            //     printf("exec failed: file not found!");
            //     continue;
            // }
   		}
	}

	return 0;

}