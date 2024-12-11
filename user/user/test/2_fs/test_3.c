#include <stdio.h>
#include <string.h>

#define BUF_SIZE 4096 // 4KB
// #define BUF_SIZE 8196		//8KB
// #define BUF_SIZE 16384	//16KB
// #define BUF_SIZE 32768	//32KB
// #define BUF_SIZE 65536	//64KB

#define DATA_SIZE 200

char bufw[BUF_SIZE];
char bufr[BUF_SIZE];

int main() {
    int fd;
    char filename[] = "text1.txt";

    for (int i = 0; i < BUF_SIZE; i += 10) {
        for (int j = 0; j < 10; j++) { bufw[i + j] = '0' + (char)j; }
    }

    fd = open(filename, O_CREAT | O_RDWR, I_RW);
    if (fd != -1) {
        write(fd, bufw, DATA_SIZE);
        write(fd, bufw + DATA_SIZE, 51);
        write(fd, bufw + DATA_SIZE + 51, 210);

        close(fd);
    } else {
        exit(-1);
        return -1;
    }

    fd = open(filename, O_RDWR);
    if (fd != -1) {
        read(fd, bufr, DATA_SIZE);
        read(fd, bufr + DATA_SIZE, 51);
        read(fd, bufr + DATA_SIZE + 51, 210);
        // udisp_str(bufr);
        close(fd);
        printf("the bufr:%s\n", bufr);
    } else {
        return -1;
    }

    fd = open("text1b.txt", O_CREAT | O_RDWR, I_RW);
    if (fd != -1) {
        write(fd, bufr, strlen(bufr));
        close(fd);
    } else {
        return -1;
    }

    exit(0);
}
