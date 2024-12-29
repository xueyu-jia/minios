// 测试 get_pid_byname 能否正确返回当前进程的 pid

#include <usertest.h>

const char *test_name = "pidbyname01";
const char *syscall_name = "get_pid_byname";

logging logger;

const char *process_name = "pidbyname01"; // 当前程序的名字

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int pid_byname = get_pid_byname("pidbyname01");
    int pid = get_pid();
    if (pid_byname != pid) {
        info(&logger, "get_pid_byname retrun %d, expected: %d\n", pid_byname, pid);
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
