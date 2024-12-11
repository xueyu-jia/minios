/*
 * 要创建的目录已经存在时，测试 mkdir 能否正确返回 DIR_PATH_REPEATED i.e. -2
 *
 * do not use opendir in fat32 for it is deprecated, use chdir instead
 * orangefs do not implement opendir, readdir and chdir
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
 * DIR_PATH_INEXISTE    -1    // 路径不存在
 * DIR_PATH_REPEATED    -2    // 目录已存在
 * DIR_FILE_OCCUPIYED   -3    // 目录下存在文件
 */

#include <usertest.h>

const char *test_name = "mkdir02";
const char *syscall_name = "mkdir";

logging logger;

const char *dirname = "dir02";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    // 预先创建目录
    int rval = mkdir(dirname, I_RWX);
    if (rval != 0) {
        info(&logger, "setup(): return %d\n", rval);
        exit(-1);
    }
}

void cleanup() {
    rmdir(dirname);
    logger_close(&logger);
}

void run() {
    int rval = mkdir(dirname, I_RWX);
    info(&logger, "mkdir %s return %d, expected %d\n", dirname, rval,
         DIR_PATH_REPEATED);
    if (rval != DIR_PATH_REPEATED) {
        cleanup();
        exit(-1);
    }
    info(&logger, "test passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
