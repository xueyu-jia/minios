#include "../include/stdio.h"
#include "../include/pthread.h"
#include "../include/syscall.h"

#define BUF_SIZE 10

char bufw1[BUF_SIZE] = "hello ";
char bufw2[BUF_SIZE] = "word ";
char bufw3[BUF_SIZE] = "xxxx ";
char bufr[BUF_SIZE];
pthread_t producer1, producer2, producer3, consumer1, consumer2;

void produce1(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw1, strlen(bufw1));
    }
}

void produce2(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw2, strlen(bufw2));
    }
}

void produce3(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw3, strlen(bufw3));
    }
}

void consume(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 75; i++) {
        read(fd, bufr, BUF_SIZE);
        udisp_str(bufr);
    }
}


int main(int arg, char *argv[])
{
    int fd;
	char filename[] = "fat0/test33.txt";

    fd = create(filename);
    if (fd == -1) {
        printf("open error!");
        exit(-1);
    }

	pthread_create(&producer1, NULL, produce1, &fd);
	pthread_create(&producer2, NULL, produce2, &fd);
	pthread_create(&producer3, NULL, produce3, &fd);
	pthread_create(&consumer1, NULL, consume, &fd);
	pthread_create(&consumer2, NULL, consume, &fd);

	sleep(200);
    exit(0);

	return 0;
}