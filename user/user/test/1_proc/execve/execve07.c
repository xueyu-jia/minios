#include <usertest.h>
/*
    测试execve环境变量
*/

logging logger;

#define N_FORKS 20
const char *child_name = "child07";
const char *test_name = "execve07";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    // 自定义的环境变量参数列表---> 其实就是一个指针数组
    char *const envp[] = {"execve-envp-test", NULL};
    int ret = execve(child_name, NULL, envp);
    info(&logger, "execve return %d, exepcted %d\n", ret, 0);
    if (ret != 0) {
        info(&logger, "failed\n");
        exit(TC_FAIL);
    }
    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_FAIL);
}
