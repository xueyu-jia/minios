#include "../include/stdio.h"
#include "../include/pthread.h"
#include "../include/syscall.h"

#define BUF_SIZE 1024

char bufw1[BUF_SIZE] = "aa ";
char bufw2[BUF_SIZE] = "bb ";
char bufw3[BUF_SIZE] = "cc ";
char bufr[BUF_SIZE];
char filename[] = "test33.txt";       // orangfs 目录下
//char filename[] = "fat0/test33.txt";    // fat32 目录下
volatile int sum = 0;
pthread_mutex_t mutex;
pthread_t producer1, producer2, producer3;

void produce1(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw1, strlen(bufw1));
    }
    pthread_mutex_lock(&mutex);
    ++sum;
    pthread_mutex_unlock(&mutex);
    printf("produce1 over\n");
    while(1) {
        yield();
    }
}

void produce2(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw2, strlen(bufw2));
    }
    pthread_mutex_lock(&mutex);
    ++sum;
    pthread_mutex_unlock(&mutex);
    printf("produce2 over\n");
    while(1) {
        yield();
    }
}

void produce3(void *arg){
    int fd = *(int*)arg;
	for (int i = 0; i < 50; i++) {
        write(fd, bufw3, strlen(bufw3));
    }
    pthread_mutex_lock(&mutex);
    ++sum;
    pthread_mutex_unlock(&mutex);
    printf("produce3 over\n");
    while(1) {
        yield();
    }
}


int main(int arg, char *argv[])
{
    int fd;
    printf("filename:%s\n", filename);
    pthread_mutex_init(&mutex, NULL);

    fd = open(filename, O_CREAT | O_RDWR);
    if (fd == -1) { 
        printf("open1 error!");
        exit(-1);
    }
    pthread_create(&producer1, NULL, produce1, &fd);
	pthread_create(&producer2, NULL, produce2, &fd);
	pthread_create(&producer3, NULL, produce3, &fd);

    while(sum < 3) {
        yield();
    }
    printf("\nWRITE OVER\n");
    close(fd);

    fd = open(filename, O_RDWR);
    if (fd == -1) { 
        printf("open2 error!");
        exit(-1);
    } else {
        read(fd, bufr, BUF_SIZE);
        u32 len = strlen(bufr);
        bufr[BUF_SIZE-1] = '\0';
        printf("bufr len: %d\n", len);
        printf("bufr is: %s\n", bufr);
    }
    close(fd);
    exit(0);

	return 0;
}