#include <stdio.h>
#include <string.h>
/*
 * orangfs文件系统下
 * 长路径
 * 测试系统调用
 * 创建、删除目录
 * 创建、删除文件
 * 文件读写
 * 打开文件、关闭文件
 *
 */

int user_test(int order, int val, int is_true, char* test_messege);
void main(int argc, char* argv[]) {
    int test_order = 0;
    if (fork() == 0) {
        // 子进程一直循环，父进程读写硬盘产生的中断实际上会在子进程运行时触发
        // 如果报错：SATA handler:No error but interrupt
        // 说明子进程没有映射sata寄存器的端口（AHCI的端口）
        // v1.5.2之后子进程改为可以正常退出，
        // 所有进程都在sleep的情况下，cfs调度会唤醒idle进程(pid=0)
        // 2024.5 jiangfeng
        child_function();
        // while(1){};
        exit(0);
    }
    // 创建长目录
    char path[128], buff[128], filename[128];
    int ret, fd;
    memset(path, 0, 128);
    memset(filename, 0, 128);
    memset(buff, 0, 128);
    int i;
    for (i = 0; i < 118; i++) {
        path[i++] = '/';
        path[i++] = 'a' + (i % 26);
        path[i++] = 'a' + (i % 26);
        path[i++] = 'a' + (i % 26);
        path[i++] = 'a' + (i % 26);
        path[i++] = 'a' + (i % 26);
        path[i] = 'a' + (i % 26);
        ret = mkdir(path, I_RWX);
        user_test(test_order++, ret, ret == 0, "mkdir");
    }
    printf("path:%s\n", path);
    printf("path length :%d\n", strlen(path));
    // 在长目录下创建文件并写入数据
    strcpy(filename, path);
    filename[i++] = '/';
    while (i < 127) { filename[i++] = 'A' + (i % 26); }
    filename[i] = 0;
    printf("path + filename length :%d\n", strlen(filename));
    fd = open(filename, O_CREAT | O_RDWR, I_R | I_W);
    user_test(test_order++, fd, fd >= 0, "open file");
    if (fd >= 0) {
        ret = write(fd, filename, strlen(filename));
        user_test(test_order++, ret, ret == strlen(filename), "write data");
        ret = close(fd);
        user_test(test_order++, ret, ret == 0, "close the file");
    }

    // 打开文件并读入数据
    fd = open(filename, O_RDWR);
    user_test(test_order++, fd, fd >= 0, "open the file");
    ret = read(fd, buff, 128);
    user_test(test_order++, ret, ret >= 0, "read the file");
    ret = close(fd);
    user_test(test_order++, ret, ret == 0, "close the file");
    ret = strncmp(filename, buff, strlen(filename));
    user_test(test_order++, ret, ret == 0, "write and read cmp");

    printf("file content: \n%s\n", buff);
    printf("content length :%d\n", strlen(buff));

    // 删除不存在的目录
    ret = rmdir("errordir");
    user_test(test_order++, ret, ret != 0, "delete error dir");

    // 删除有文件的目录
    ret = rmdir(path);
    // for(int j = 0; j < 100000000; j++);
    user_test(test_order++, ret, ret != 0, "delete a dir which have files");

    // 删除文件,然后再删除目录
    ret = unlink(filename);
    // for(int j = 0; j < 100000000; j++);
    user_test(test_order++, ret, ret == 0, "delete file");

    ret = rmdir(path);
    // for(int j = 0; j < 100000000; j++);
    user_test(test_order++, ret, ret == 0, "delete empty dir");

    printf("FINISHED!!!\n");
    exit(0);
    return 0;
}

int user_test(int order, int val, int is_true, char* test_messege) {
    printf("test %d: %s ", order, test_messege);
    if (is_true) {
        printf("SUCCEED!\n");
    } else {
        printf("ERROR VAL: %d\n", val);
        printf("\nfstest error!!\n\n");
        exit(-1);
    }
    return is_true;
}

int child_function() {
    char child_filename[] = "child_file";
    char child_buf[128];
    int ret, fd;
    int test_order = 0;

    fd = open(child_filename, O_CREAT | O_RDWR, I_RW);
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
    ret = strncmp(child_filename, child_buf, strlen(child_filename));
    user_test(test_order++, ret, ret == 0, "child write and read cmp");

    printf("CHILD FINISHED!!!\n");
    // while(1);
}
