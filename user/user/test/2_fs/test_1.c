#include <stdio.h> //modified by mingxuan 2021-8-29
#include <string.h>

int user_test(int order, int val, int is_true, char *test_messege) {
    printf("test %d: %s ", order, test_messege);
    if (is_true) {
        printf("SUCCEED!\n");
    } else {
        printf("ERROR VAL: %d\n", val);
        printf("\n!!!TEST_1 ERROR!!!\n\n");
        exit(-1);
    }
    return is_true;
}

int test1_function(char *child_filename) {
    // char child_filename[] = "child_file";
    char child_buf[128];
    int ret, fd;
    int test_order = 0;

    // 创建文件
    fd = open(child_filename, O_CREAT | O_RDWR, I_RWX);
    user_test(test_order++, fd, fd >= 0, "child open file");
    if (fd >= 0) {
        ret = write(fd, child_filename, strlen(child_filename));
        user_test(test_order++, ret, ret == strlen(child_filename),
                  "child write data");
        ret = close(fd);
        user_test(test_order++, ret, ret == 0, "child close the file");
    }

    // 打开文件并读入数据
    fd = open(child_filename, O_RDWR);
    user_test(test_order++, fd, fd >= 0, "child open the file");
    ret = read(fd, child_buf, 128);
    user_test(test_order++, ret, ret >= 0, "child read the file");
    ret = close(fd);
    user_test(test_order++, ret, ret == 0, "child close the file");

    printf("write content:%s\n", child_filename);
    printf("read contend:%s\n", child_buf);
    ret = strncmp(child_filename, child_buf, strlen(child_filename));
    user_test(test_order++, ret, ret == 0, "child write and read cmp");

    printf("CHILD FINISHED!!!\n");
    // while(1);
}

void main(int argc, char *argv[]) {
    test1_function("filename");
    exit(0);
    return;
}
