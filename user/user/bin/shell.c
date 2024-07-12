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
// 全局便于使用，每条命令解析时更新
char disk_cmd_path[MAX_PATH];
char *current_cmd; // 实际内存位于栈(main.buff)

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

int check_disk_command(const char* path) {
	struct stat statbuf;
	int status, _fd;
	char sample_buff[MAX_PATH];
	status = stat(path, &statbuf);
	if(status == 0 && statbuf.st_type == I_REGULAR){
		char *extname = strrchr(path, '.');
		if(strcmp(extname, ".sh") == 0) {
			return CMD_SCRIPT;
		}
		_fd = open(path, O_RDONLY);
		lseek(_fd, 0, SEEK_SET);
		fgets(sample_buff, MAX_PATH, _fd);
		close(_fd);
		if(sample_buff[1]=='E' && sample_buff[2]=='L' && sample_buff[3]=='F') {
			return CMD_ELF;
		}
	}
	return CMD_INVAL;
}

// 搜索cmd并更新可执行文件的路径至disk_cmd_path
int search_command_path(const char * cmd) {
	current_cmd = cmd;
	char *path = getenv("PATH"), *pstr;
	char *completed = disk_cmd_path;
	int result;
	// 存在'/',直接传递路径，VFS会处理相对cwd和绝对路径两种情况
	if(strchr(cmd, '/') != NULL) {
		strcpy(completed, cmd);
		return check_disk_command(cmd);
	}
	// 路径只有文件名，要从PATH搜索，并拼接路径
	while(path != NULL) {
		pstr = strchr(path, ';');
		if(pstr == 0)pstr = path + strlen(path);
		strncpy(completed, path, pstr-path);
		completed[pstr-path] = 0;
		strcat(completed, "/");
		strcat(completed, cmd);
		result = check_disk_command(completed);
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
	// builtin
	int cmdid = match_build_in(argc, args);
	if(cmdid != -1){
		int ret = cmds[cmdid].handler(argc, args);
		if(ret != 0){
			printf("%s exit %d", cmds[cmdid].cmd_name, ret);
		}
		return;
	}
	// test cmd exec file
	// 此函数根据PATH环境变量查找得到真正的可执行文件的类型和实际路径(cmd)
	int cmd_type = search_command_path(args[0]);
	if(CMD_INVAL == cmd_type) {
		fprintf(STD_ERR, "command not found:%s\n", args[0]);
		return;
	}else if(CMD_SCRIPT == cmd_type) {
		// 此命令是脚本类型，cmd实际是interpreter的参数
		args[1] = args[0];
		args[0] = "/bin/shell";
		if(CMD_ELF != search_command_path(args[0])) {
			fprintf(STD_ERR, "%s:script interpreter not executable!\n", args[0]);
			return;
		}
	}
	// CMD_ELF
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
		if(CMD_ELF == cmd_type) {
			printf("%s(pid:%d): exit_status:%d\n", current_cmd, pid, exit_status);
		}
	}
	else
	{	//child
		if(execve(disk_cmd_path, args, env)!=0)
		{
			fprintf(STD_ERR, "%s: exec failed!\n", disk_cmd_path);
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
		if(CMD_SCRIPT != search_command_path(argv[1])) {
			fprintf(STD_ERR, "not a script %s\n", argv[1]);
			return -1;
		}
		fd = open(argv[1], O_RDONLY);
		if(fd < 0) {
			fprintf(STD_ERR, "fail to open script %s\n", argv[1]);
			return -1;
		}
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
			break; // script shell exit
			fd = STD_IN;
			// while(1);
		}
		if(len != 0 && buf[0] != '#')
		{
			// printf("cmd: %s\n", buf);
			int argc = parse(buf, args, MAX_ARGC);
			do_command(argc, args);
   		}

	}
	return 0;
}