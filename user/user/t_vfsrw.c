#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "opt.h"

/**
 * t_vfsrw.c VFS 文件数据读写测试
 * 测试环境：测试程序运行当前目录
 * 用法：t_vfsrw -[bftr]
 * 选项：-b blocknum 测试读写的文件大小，以4K字节块单位，默认256块(1M)
 *      -f name 指定测试文件的文件名，默认为"test_file"
 *      -r 只读取指定测试文件并检查内容 
 * 主要流程：
 * 1. 创建测试文件，并写入
*/

#define EXPECT_ZERO(exp, testinfo)  if((exp) != 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NONZERO(exp, testinfo)  if((exp) == 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NONNEG(exp, testinfo)  if((exp) < 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NEG(exp, testinfo)  if((exp) >= 0){printf("%s failed\n", #testinfo);exit(-1);}

struct stat statbuf;
char buf[4096];
char *test_name = "test_file";

int check_filesize() {
	int st = stat(test_name, &statbuf);
    EXPECT_ZERO(st, stat file);
    return statbuf.st_size;
}

int check_content(int order) {
    char c = ('0'+order)%256;
    if(((int*)buf)[0] == order){
        for(int i = 4; i < 4096; i++){
            if(buf[i] != c){
                printf("file wrong at %d\n", order*4096 + i);
                return -1;
            }
        }
        return 0;
    }
    printf("file wrong at %d\n", order*4096);
    return -1;
}

int main(int argc, char **argv) {
    int test_block = 256;
    
    int opt, show_time = 1, read_only = 0;
    while((opt = getopt(argc, argv, "b:f:r")) != -1) {
        switch (opt)
        {
        case 'b':
            test_block = atoi(optarg);
            break;
        
        case 'f':
            test_name = optarg;
            break;
        
        case 'r':
            read_only = 1;
            break;
        default:
            break;
        }
    }
    int write_usage = 0, read_usage = 0, tmp_tick = 0, cnt;
    int fd;
    if(!read_only) 
    {

        fd = open(test_name, O_CREAT|O_RDWR, I_RWX);
        EXPECT_NONNEG(fd, create and open test);
        lseek(fd, 0, SEEK_SET);
        // write
        for(int i = 0; i < test_block; i++) {
            memset(buf, ('0'+i)%256, 4096);
            *((int*)buf) = i;
            tmp_tick = get_ticks();
            cnt = write(fd, buf, 4096);
            write_usage += get_ticks() - tmp_tick;
            EXPECT_NONZERO(cnt == 4096, write buf count);
        }
        close(fd);
        printf("write ok, usage: %d\n", write_usage);
    }
    EXPECT_NONZERO((check_filesize() == (test_block*4096)), check filesize);
    fd = open(test_name, O_RDONLY);
    lseek(fd, 0, SEEK_SET);
    // read and check content
    for(int i = 0; i < test_block; i++) {
        tmp_tick = get_ticks();
        cnt = read(fd, buf, 4096);
        read_usage += get_ticks() - tmp_tick;
        EXPECT_NONZERO(cnt == 4096, read buf count);
        EXPECT_ZERO(check_content(i), read content);
    }
    close(fd);
    printf("read pass, usage: %d\n", read_usage);
    return 0;
}