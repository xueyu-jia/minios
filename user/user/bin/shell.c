#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include "env.h"
#define MAX_ARGC	8
#define NUM_BUILTIN_CMD	3
#define CMD_LEN	8

#define CMD_ELF 0
#define CMD_SCRIPT 1
#define CMD_INVAL -1

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

int do_free(int argc, char** argv) {
    u32 free_mem = total_mem_size();
	printf("free_mem:%x\n",free_mem);
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

int _test_command(const char* path) {
	struct stat statbuf;
	int status, _fd;
	char sample[4];
	status = stat(path, &statbuf);
	if(status == 0 && statbuf.st_type == I_REGULAR){
		char *extname = strrchr(path, '.');
		if(strcmp(extname, ".sh") == 0) {
			return CMD_SCRIPT;
		}
		_fd = open(path, O_RDONLY);
		lseek(_fd, 0, SEEK_SET);
		read(_fd, sample, 4);
		close(_fd);
		if(sample[1]=='E' && sample[2]=='L' && sample[3]=='F') {
			return CMD_ELF;
		}
	}
	return CMD_INVAL;
}

int test_command_path(char *completed, const char * cmd) {
	char *path = getenv("PATH"), *pstr;
	int result;
	// 存在'/',直接传递路径，VFS会处理相对cwd和绝对路径两种情况
	if(strchr(cmd, '/') != NULL) {
		return _test_command(cmd);
	}
	// 路径只有文件名，要从PATH搜索，并拼接路径
	while(path != NULL) {
		pstr = strchr(path, ';');
		if(pstr == 0)pstr = path + strlen(path);
		strncpy(completed, path, pstr-path);
		completed[pstr-path] = 0;
		strcat(completed, "/");
		strcat(completed, cmd);
		result = _test_command(completed);
		if(CMD_INVAL != result) {
			return result;
		}
		if(*pstr == NULL){
			break;
		}
		path = pstr + 1;
	}
	return CMD_INVAL;
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
	// 此函数根据PATH环境变量查找得到真正的可执行文件的类型和实际路径(cmd)
	int cmd_type = test_command_path(cmd, args[0]);
	if(CMD_INVAL == cmd_type) {
		printf("command not found:%s\n", args[0]);
		return;
	}else if(CMD_SCRIPT == cmd_type) {
		// 此命令是脚本类型，打开脚本文件并将全局fd置为打开的文件描述符
		// 直接返回，进行脚本文件内容的读取
		fd = open(cmd, O_RDONLY);
		return;
	} 
	// CMD_ELF == cmd_type
	char* simple_cmd = args[0];
	args[0] = cmd;
	int pid = fork();
	// printf("%d: fork return %d\n", get_pid(), pid);
	if(pid != 0)
	{	//father
		if(pid < 0) {
			fprintf(STD_ERR, "fork failed\n");
			return;
		}
       	int exit_status;
        wait(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
		printf("%s(pid:%d): exit_status:%d\n", simple_cmd, pid, exit_status);
	}
	else
	{	//child
		if(execve(cmd, args, env)!=0)
		{
			printf("exec failed: file not found!");
          	exit(-1);
       	}
		fprintf(STD_ERR, "unreachable area\n");
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
	// printf("argc:%d\n", arg);
    // printstring("argv:", argv);
    // printstring("env:", envp);
	env = envp;
    char buf[BUF_SIZE];
	char pwd[MAX_PATH];
    int len;
	char * args[MAX_ARGC];
	reg_cmd("cd", do_cd);
	reg_cmd("pwd", do_pwd);
	reg_cmd("free", do_free);
	nice(1);
	#ifdef SHELL_TEST
	#define TEST_CMD_LEN_LIMIT	32

	#define TEST_CMD_NUM 2
	int pre = 0;
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
			// printf("input: %s\n", buf);
			int argc = parse(buf, args, MAX_ARGC);
			do_command(argc, args);
   		}

	}
	return 0;
}