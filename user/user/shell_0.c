#include "stdio.h"
#include "string.h"
#define MAX_ARGC	8
#define NUM_BUILTIN_CMD	5
#define CMD_LEN	8
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
	return mkdir(argv[1]);
}

int do_mount(int argc, char** argv){
	if(argc != 4){
		printf("usage: mount %%dev %%target %%fstype\n");
		return -1;
	}
	return mount(argv[1], argv[2], argv[3], 0, NULL);
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

void main(int arg,char *argv[])
{
    /*
	int stdin = open("dev_tty0",O_RDWR);
	int stdout= open("dev_tty0",O_RDWR);
	int stderr= open("dev_tty0",O_RDWR);
	*/

    char buf[1024];
	char pwd[MAX_PATH];
    int pid;
    int times = 0;
	char * args[MAX_ARGC];
	reg_cmd("ls", do_ls);
	reg_cmd("cd", do_cd);
	reg_cmd("pwd", do_pwd);
	reg_cmd("mkdir", do_mkdir);
	reg_cmd("mount", do_mount);
  	while(1)
	{
        printf("\nminiOS:%s $ ",getcwd(pwd, MAX_PATH));
        if(gets(buf) && strlen(buf)!=0 )
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

	return ;

}