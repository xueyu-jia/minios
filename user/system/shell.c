#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include "env.h"
#define MAX_ARGC	8
#define NUM_BUILTIN_CMD	2
#define CMD_LEN	8

// #define SHELL_TEST

struct cmd{
	char cmd_name[CMD_LEN];
	int (*handler)(int , char**);
};

struct cmd cmds[NUM_BUILTIN_CMD];
int num_cmd;
int fd = STD_IN;
char **env;

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

int test_command(char *completed, const char * cmd) {
	char *path = getenv("PATH"), *pstr;
	int status, _fd;
	char sample[4];
	struct stat statbuf;
	while(path != NULL) {
		pstr = strchr(path, ';');
		if(pstr == 0)pstr = path + strlen(path);
		strncpy(completed, path, pstr-path);
		completed[pstr-path] = 0;
		strcat(completed, "/");
		strcat(completed, cmd);
		status = stat(completed, &statbuf);
		if(status == 0 && statbuf.st_type == I_REGULAR){
			char *extname = strrchr(completed, '.');
			if(strcmp(extname, ".sh") == 0) {
				return 1;
			}
			_fd = open(completed, O_RDONLY);
			lseek(_fd, 0, SEEK_SET);
			read(_fd, sample, 4);
			close(_fd);
			if(sample[1]=='E' && sample[2]=='L' && sample[3]=='F') {
				return 0;
			}
		}
		if(*pstr == NULL){
			break;
		}
		path = pstr + 1;
	}
	return -1;
}

void do_command(int argc, char** args) {
	char cmd[256];
	int cmdid = match_build_in(argc, args);
	if(cmdid != -1){
		int ret = cmds[cmdid].handler(argc, args);
		printf("%s exit %d", cmds[cmdid].cmd_name, ret);
		return;
	}
	// test cmd exec file
	int rt = test_command(cmd, args[0]);
	if(-1 == rt) {
		printf("command not found:%s\n", args[0]);
		return;
	}else if(1 == rt) {
		fd = open(cmd, O_RDONLY);
		return;
	}
	args[0] = cmd;
	int pid = fork();
	if(pid!=0)
	{	//father
       	int exit_status;
        wait(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
		printf("pid:%d exit_status:%d\n", pid, exit_status);
	}
	else
	{	//child
		if(execve(cmd, args, env)!=0)
		{
			printf("exec failed: file not found!");
          	exit(-1);
       	}
	}	
}

int main(int arg,char *argv[],char *envp[])
{
#define BUF_SIZE	512
    /*
	int stdin = open("dev_tty0",O_RDWR);
	int stdout= open("dev_tty0",O_RDWR);
	int stderr= open("dev_tty0",O_RDWR);
	*/
	printf("argc:%d\n", arg);
    printstring("argv:", argv);
    printstring("env:", envp);
	env = envp;
    char buf[BUF_SIZE];
	char pwd[MAX_PATH];
    int pid, len;
    int times = 0;
	int pre = 0;
	char * args[MAX_ARGC];
	reg_cmd("cd", do_cd);
	reg_cmd("pwd", do_pwd);
	#ifdef SHELL_TEST
	#define TEST_CMD_LEN_LIMIT	32

	#define TEST_CMD_NUM 2
	char pre_test_cmds[TEST_CMD_NUM][TEST_CMD_LEN_LIMIT] = {
		"mkdir test",
		"rm -r test",
		// "cd ora",
		// "/t_xv6.bin",
	};
	
	pre = TEST_CMD_NUM;
	#endif
	if(arg > 1) {
		do_command(arg - 1, &argv[1]);
		return 0;
	}
  	while(1)
	{
		if(fd == STD_IN) {
			printf("\nminiOS:%s $ ",getcwd(pwd, MAX_PATH));
		}
        
		#ifdef SHELL_TEST
		if(pre){
			strcpy(buf, pre_test_cmds[TEST_CMD_NUM - pre]);
			printf("%s\n", buf);
			pre--;
		}else{
		#endif
			
		fgets(buf, BUF_SIZE, fd);
		#ifdef SHELL_TEST
		}
		#endif
		len = strlen(buf);
		if(fd != STD_IN && len == 0){
			// printf("%d", fd);
			close(fd);
			fd = STD_IN;
			// while(1);
		}
		if(len != 0 && buf[0] != '#')
		{
			printf("input: %s\n", buf);
			int argc = parse(buf, args, MAX_ARGC);
			do_command(argc, args);
   		}

	}
	return 0;
}