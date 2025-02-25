#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_ARGC 8
#define NUM_BUILTIN_CMD 3
#define CMD_LEN 8

enum {
    CMD_ELF,
    CMD_SCRIPT,
    CMD_INVAL,
};

struct cmd {
    char cmd_name[CMD_LEN];
    int (*handler)(int, char **);
};

const char *path_to_shell = NULL;
char **env = NULL;

int num_cmd = 0;
struct cmd cmds[NUM_BUILTIN_CMD] = {};

int last_exit_code = 0;

char disk_cmd_path[MAX_PATH] = {};
char *cur_cmd = NULL;

int do_cd(int argc, char **argv) {
    char *path = argv[1];
    return chdir(path);
}

int do_pwd(int argc, char **argv) {
    char path[MAX_PATH];
    getcwd(path, MAX_PATH);
    printf("%s", path);
    return 0;
}

int do_free(int argc, char **argv) {
    const size_t total_mem = total_mem_size();
    printf("available memory: %d %.3fKiB %.3fMiB", total_mem, total_mem * 1.0 / (1 << 10),
           total_mem * 1.0 / (1 << 20));
    return 0;
}

void reg_cmd(char *name, int (*handler)(int, char **)) {
    if (num_cmd >= NUM_BUILTIN_CMD) { return; }
    strcpy(cmds[num_cmd].cmd_name, name);
    cmds[num_cmd].handler = handler;
    num_cmd++;
}

int parse(char *buf, char **argv, int limit) {
    if (!buf) { return 0; }
    int start = 0, cnt = 0;
    char *p = buf;
    for (; *p && *p != '\n'; ++p) {
        if (*p == ' ' && start == 1) {
            *p = 0;
            start = 0;
        } else if (start == 0 && (*p != ' ') && cnt < limit) {
            *argv++ = p;
            cnt++;
            start = 1;
        }
    }
    if (start) *p = 0;
    *argv = 0;
    return cnt;
}

int match_build_in(int argc, char **argv) {
    for (int i = 0; i < NUM_BUILTIN_CMD; ++i) {
        if (argc && (!strcmp(argv[0], cmds[i].cmd_name))) { return i; }
    }
    return -1;
}

int check_disk_command(const char *path) {
    struct stat statbuf = {};
    int status = 0;
    int fd = -1;
    char sample_buff[MAX_PATH];
    status = stat(path, &statbuf);
    if (status == 0 && statbuf.st_type == I_REGULAR) {
        char *extname = strrchr(path, '.');
        if (strcmp(extname, ".sh") == 0) { return CMD_SCRIPT; }
        fd = open(path, O_RDONLY);
        lseek(fd, 0, SEEK_SET);
        fgets(sample_buff, MAX_PATH, fd);
        close(fd);
        if (sample_buff[1] == 'E' && sample_buff[2] == 'L' && sample_buff[3] == 'F') {
            return CMD_ELF;
        }
    }
    return CMD_INVAL;
}

int search_command_path(const char *cmd) {
    cur_cmd = cmd;
    char *path = getenv("PATH"), *pstr;
    char *completed = disk_cmd_path;
    int result;
    if (strchr(cmd, '/') != NULL) {
        strcpy(completed, cmd);
        return check_disk_command(cmd);
    }
    while (path != NULL) {
        pstr = strchr(path, ';');
        if (pstr == 0) pstr = path + strlen(path);
        strncpy(completed, path, pstr - path);
        completed[pstr - path] = 0;
        strcat(completed, "/");
        strcat(completed, cmd);
        result = check_disk_command(completed);
        if (CMD_INVAL != result) { return result; }
        if (*pstr == NULL) { break; }
        path = pstr + 1;
    }
    return CMD_INVAL;
}

bool might_is_regular_file(const char *pathname) {
    int fd = open(pathname, O_RDWR);
    if (fd < 0) { return false; }
    close(fd);
    return true;
}

void do_command(int argc, char **args) {
    //! cd command shortcut
    if (argc == 1) {
        const char *item = args[0];
        bool matched = true;
        do {
            if (strcmp(item, ".") == 0 || strcmp(item, "..") == 0) { break; }
            if (strstr(item, "/") == item || strstr(item, "./") == item ||
                strstr(item, "../") == item) {
                break;
            }
            matched = false;
        } while (0);
        if (matched && !might_is_regular_file(item)) {
            char *args[2] = {"cd", item};
            int retval = do_cd(2, args);
            if (retval != 0) { printf("error: no such file or directory: %s", item); }
            last_exit_code = retval;
            return;
        }
    }

    //! builtin var to get last error code
    if (argc == 1 && strcmp(args[0], "$?") == 0) {
        printf("%d", last_exit_code);
        last_exit_code = 0;
        return;
    }

    //! builtin command
    int cmdid = match_build_in(argc, args);
    if (cmdid != -1) {
        int ret = cmds[cmdid].handler(argc, args);
        if (ret != 0) { printf("error: %s exited with status %d", cmds[cmdid].cmd_name, ret); }
        last_exit_code = ret;
        return;
    }

    //! detect command type and preprocess
    const int cmd_type = search_command_path(args[0]);
    switch (cmd_type) {
        case CMD_SCRIPT: {
            args[1] = args[0];
            args[0] = path_to_shell;
        } break;
        case CMD_ELF: {
        } break;
        default: {
            fprintf(stderr, "error: command not found: %s\n", args[0]);
            last_exit_code = -1;
            return;
        } break;
    }

    //! run executable
    const pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "error: ran out of process");
        last_exit_code = -1;
        return;
    }
    if (pid > 0) {
        int exit_status = 0;
        wait(&exit_status);
        last_exit_code = exit_status;
    } else if (execve(disk_cmd_path, args, env) != 0) {
        fprintf(stderr, "error: failed to spawn process: %s", disk_cmd_path);
        exit(-1);
    } else {
        fprintf(stderr, "error: unknown error");
        exit(-1);
    }
}

#include <stdlib.h>
#include <time.h>

void cleanup_tmpdir() {
    {
        DIR *dir = opendir("/tmp");
        if (!dir) { return; }
        closedir(dir);
    }
    if (fork() == 0) {
        execl("/bin/rm", "/bin/rm", "-r", "/tmp", NULL);
        exit(-1);
    }
    wait(NULL);
}

int main(int argc, char *argv[], char *envp[]) {
#define BUF_SIZE 512

    env = envp;
    path_to_shell = argv[0];

    char cmdbuf[BUF_SIZE] = {};
    char cwdbuf[MAX_PATH] = {};
    char *exec_argv[MAX_ARGC] = {};

    reg_cmd("cd", do_cd);
    reg_cmd("pwd", do_pwd);
    reg_cmd("free", do_free);
    nice(1);

    int fd_in = stdin;
    if (argc > 1) {
        if (CMD_SCRIPT != search_command_path(argv[1])) {
            fprintf(stderr, "error: file is not a shell script: %s\n", argv[1]);
            return -1;
        }
        fd_in = open(argv[1], O_RDONLY);
        if (fd_in < 0) {
            fprintf(stderr, "error: failed to open script: %s\n", argv[1]);
            return -1;
        }
    }

    while (true) {
        if (fd_in == stdin) { printf("\nminiOS:%s $ ", getcwd(cwdbuf, MAX_PATH)); }
        fgets(cmdbuf, BUF_SIZE, fd_in);
        const size_t len = strlen(cmdbuf);
        if (fd_in != stdin && len == 0) {
            close(fd_in);
            break;
        }
        if (len == 0) { continue; }
        if (cmdbuf[0] == '#') { continue; }
        const int argc = parse(cmdbuf, exec_argv, MAX_ARGC);
        do_command(argc, exec_argv);
    }
    return 0;
}
