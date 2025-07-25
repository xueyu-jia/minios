#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

void print_time(uint32_t timestamp) {
    char buf[32] = {0};
    struct tm time = {0};
    localtime(timestamp, &time);
    strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &time);
    printf("%s", buf);
}

char *inodetype2str(int type) {
    switch (type) {
        case I_REGULAR:
            return "Regular file";
        case I_DIRECTORY:
            return "Directory";
        case I_CHAR_SPECIAL:
            return "Char special file";
        case I_BLOCK_SPECIAL:
            return "Block special file";
        default:
            break;
    }
    return "Unknown";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: stat %%path\n");
        return -1;
    }
    struct stat *pstatbuf = (struct stat *)malloc(sizeof(struct stat));
    int st = stat(argv[1], pstatbuf);
    if (st != 0) {
        printf("%s not found!", argv[1]);
        return -1;
    }
    printf("File name:%s\n", argv[1]);
    printf("size:%d  inode:%d  type:%s  ", (uint32_t)pstatbuf->st_size, pstatbuf->st_ino,
           inodetype2str(pstatbuf->st_type));
    if (pstatbuf->st_type == I_BLOCK_SPECIAL || pstatbuf->st_type == I_CHAR_SPECIAL) {
        uint32_t dev = pstatbuf->st_rdev;
        printf("device: %d,%d", dev >> 20, dev & 0x0FFFFF);
    }
    printf("\nLast access:");
    print_time(pstatbuf->st_atim);
    printf("\nLast modified:");
    print_time(pstatbuf->st_mtim);
    printf("\nCreation:");
    print_time(pstatbuf->st_crtim);
    printf("\n");
    free(pstatbuf);
    return 0;
}
