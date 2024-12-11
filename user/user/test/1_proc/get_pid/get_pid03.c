/*
 *  多次 fork，检查进程 get_pid 是否都返回不同的 pid 值。
 *
 *  1. 多次 fork
 *  2. 各个子进程返回自己的 pid 值
 *  3. 父进程等待所有子进程结束
 *  4. 检查返回的 pid 值是否是独特的
 *
 */

#include <usertest.h>

const char *test_name = "get_pid02";
const char *syscall_name = "get_pid";

logging logger;

#define N_FORKS 40

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// return true if each value in pids is unique
int check_unique(int pids[], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            if (pids[i] == pids[j]) {
                error(&logger, "pids[%d] = %d, pids[%d] = %d\n", i, pids[i], j,
                      pids[j]);
                return 0; // return false
            }
        }
    }
    return 1; // return true
}

void run() {
    int pids[N_FORKS + 1];

    info(&logger, "fork...\n");
    for (int i = 0; i < N_FORKS; i++) {
        int pid = SAFE_FORK();
        if (pid == 0) {      // child process
            exit(get_pid()); // 返回 pid
        }
    }

    // father process
    pids[N_FORKS] = get_pid(); // 父进程自己的 pid
    info(&logger, "waiting children exiting...\n");
    for (int i = 0; i < N_FORKS; i++) {
        int status = -1;
        wait(&status);
        pids[i] = status;
    }

    if (!check_unique(pids, N_FORKS + 1)) {
        error(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "all pids are unique\n");
    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
