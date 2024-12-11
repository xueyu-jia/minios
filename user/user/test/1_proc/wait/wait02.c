/*
 * 测试 wait 是否能正确返回子进程的 pid 和状态
 *
 */

#include <usertest.h>

const char *test_name = "wait02";
const char *syscall_name = "wait";

logging logger;

const int child_status = 124;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    info(&logger, "fork\n");
    int pid = SAFE_FORK();

    if (pid == 0) { // child process
        info(&logger, "child exiting...\n");
        exit(child_status);
    }

    // parent process
    int status;
    int child_pid = wait(&status);
    if (status != child_status) { // 接收到的状态不一致
        info(&logger, "child return status: %d, expected: %d\n", status,
             child_status);
        cleanup();
        exit(TC_FAIL);
    }

    if (pid != child_pid) { // 返回的 pid 值不正确
        info(&logger, "wait return pid: %d, expected: %d\n", child_pid, pid);
        info(&logger, "failed\n");
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
