/*
 * 测试 rmdir 能否正常删除目录
 *
 * how to determine a directory really exists in file system?
 * since we don't have stat syscall.
 *
 * orangefs implements:
 * f_op_table[1].open = real_open;
 * f_op_table[1].close = real_close;
 * f_op_table[1].write = real_write;
 * f_op_table[1].lseek = real_lseek;
 * f_op_table[1].unlink = real_unlink;
 * f_op_table[1].read = real_read;
 * f_op_table[1].mkdir = real_mkdir;
 * f_op_table[1].rmdir = real_rmdir;
 *
 * mkdir 成功时会返回 0
 * 失败时可能会返回：
 * DIR_PATH_INEXISTE    -1
 * DIR_PATH_REPEATED    -2
 * DIR_FILE_OCCUPIYED   -3
 */

#include <usertest.h>

const char *test_name = "rmdir01";
const char *syscall_name = "rmdir";

logging logger;

const char *dirname = "ddir01";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    int rval = mkdir(dirname, I_RWX);
    if (rval != 0) {
        info(&logger, "mkdir return %d\n", rval);
        exit(-1);
    }
}

void cleanup() {
    rmdir(dirname);
    logger_close(&logger);
}

void run() {
    int rval = rmdir(dirname);
    if (rval != 0) {
        error(&logger, "rmdir %s return %d, expected 0\n", dirname, rval);
        cleanup();
        exit(-1);
    }
    // TODO
    info(&logger, "rmdir %s return %d\n", dirname, rval);
    info(&logger, "test passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
