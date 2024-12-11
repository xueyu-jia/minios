#include <malloc.h>
#include <opt.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>

/**
 * t_vfsrw.c VFS 文件数据读写测试
 * 测试环境：测试程序运行当前目录
 * 用法：t_vfsrw -[bfrs]
 * 选项：-b blocknum 测试读写的文件大小，以4K字节块单位，默认256块(1M)
 *      -f name 指定测试文件的文件名，默认为"test_file"
 *      -r 只读取指定测试文件并检查内容
 *      -s 测试采用随机读写
 * 主要流程：
 * 1. 如果未指定-r, 创建测试文件，并写入数据
 * 2. 检查测试文件的大小
 * 3. 进行测试文件读测试，根据选项决定顺序读或随机读
 */

#define EXPECT_ZERO(exp, testinfo)        \
    if ((exp) != 0) {                     \
        printf("%s failed\n", #testinfo); \
        exit(-1);                         \
    }
#define EXPECT_NONZERO(exp, testinfo)     \
    if ((exp) == 0) {                     \
        printf("%s failed\n", #testinfo); \
        exit(-1);                         \
    }
#define EXPECT_NONNEG(exp, testinfo)      \
    if ((exp) < 0) {                      \
        printf("%s failed\n", #testinfo); \
        exit(-1);                         \
    }
#define EXPECT_NEG(exp, testinfo)         \
    if ((exp) >= 0) {                     \
        printf("%s failed\n", #testinfo); \
        exit(-1);                         \
    }

unsigned int seed;
int get_rand(int limit) {
    seed += get_ticks();
    seed = (10001 * seed + 1);
    return (seed % MAX_INT) % limit;
}
struct stat statbuf;
char buf[4096];
char *test_name = "test_file";

int check_filesize() {
    int st = stat(test_name, &statbuf);
    EXPECT_ZERO(st, stat file);
    return statbuf.st_size;
}

int check_content(int order) {
    char c = ('0' + order) % 256;
    if (((int *)buf)[0] == order) {
        for (int i = 4; i < 4096; i++) {
            if (buf[i] != c) {
                printf("file wrong at %d\n", order * 4096 + i);
                return -1;
            }
        }
        return 0;
    }
    printf("file wrong at %d\n", order * 4096);
    return -1;
}

int main(int argc, char **argv) {
    int test_block = 256;
    int opt, read_only = 0, shuffle = 0;
    while ((opt = getopt(argc, argv, "b:f:rs")) != -1) {
        switch (opt) {
            case 'b':
                test_block = atoi(optarg);
                break;

            case 'f':
                test_name = optarg;
                break;

            case 'r':
                read_only = 1;
                break;

            case 's':
                shuffle = 1;
            default:
                break;
        }
    }
    char *block_history = malloc(test_block);
    memset(block_history, 0, test_block);
    int write_usage = 0, read_usage = 0, tmp_tick = 0, cnt;
    int fd;
    if (!read_only) {
        fd = open(test_name, O_CREAT | O_RDWR, I_RWX);
        EXPECT_NONNEG(fd, create and open test);
        lseek(fd, 0, SEEK_SET);
        // write
        for (int i = 0; i < test_block; i++) {
            int blk = i;
            if (shuffle) {
                do { blk = get_rand(test_block); } while (block_history[blk]);
                block_history[blk] = 1;
                EXPECT_NONZERO(lseek(fd, blk * 4096, SEEK_SET) == blk * 4096,
                               random lseek);
            }
            memset(buf, ('0' + blk) % 256, 4096);
            *((int *)buf) = blk;
            tmp_tick = get_ticks();
            cnt = write(fd, buf, 4096);
            write_usage += get_ticks() - tmp_tick;
            EXPECT_NONZERO(cnt == 4096, write buf count);
        }
        tmp_tick = get_ticks();
        close(fd);
        write_usage += get_ticks() - tmp_tick;
        printf("write ok, usage: %d\n", write_usage);
    }
    EXPECT_NONZERO((check_filesize() == (test_block * 4096)), check filesize);
    memset(block_history, 0, test_block);
    fd = open(test_name, O_RDONLY);
    lseek(fd, 0, SEEK_SET);
    // read and check content
    for (int i = 0; i < test_block; i++) {
        int blk = i;
        if (shuffle) {
            do { blk = get_rand(test_block); } while (block_history[blk]);
            block_history[blk] = 1;
            EXPECT_NONZERO(lseek(fd, blk * 4096, SEEK_SET) == blk * 4096,
                           random lseek);
        }
        // printf("%d ", blk);
        tmp_tick = get_ticks();
        cnt = read(fd, buf, 4096);
        read_usage += get_ticks() - tmp_tick;
        EXPECT_NONZERO(cnt == 4096, read buf count);
        EXPECT_ZERO(check_content(blk), read content);
    }
    close(fd);
    printf("read pass, usage: %d\n", read_usage);
    free(block_history);
    return 0;
}
