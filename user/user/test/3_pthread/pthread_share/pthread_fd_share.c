/*
 *
 * by lifr 2024.10.17
一个进程（程序）中创建2-3个线程，在一个线程中使用malloc申请一些空间，
观察其他线程是否能够访问这个空间（使用这个空间中的数据），通过这个空间来进行数据通信。
 *
 *
 */

#include <usertest.h>
const char *test_name = "pthread_fd_sharing";

logging logger;

int num_threads = 3;
int shared_fd = -1;
const char *filename = "testfile.txt";
const char *write_content = "hello liferain";
char read_buffer[100];

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() {
  if (shared_fd != -1) {
    close(shared_fd);
  }
  logger_close(&logger);
}

// 线程函数1: 打开文件并写入数据
void *thread_func_open(void *arg) {
  info(&logger, "Thread open: Opening file...\n");
  info(&logger, "Thread open: Opening file...\n");
  shared_fd = open(filename, O_CREAT | O_RDWR, I_RW);
  if (shared_fd == -1) {
    info(&logger, "File open failed!\n");
    pthread_exit(NULL);
  }
  info(&logger, "Thread open: File opened, fd: %d\n", shared_fd);
  write(shared_fd, write_content, strlen(write_content));
  info(&logger, "Thread open: Wrote to file: %s\n", write_content);
  pthread_exit(NULL);
}

// 线程函数2: 读取文件内容
void *thread_func_read(void *arg) {
  while (shared_fd == -1) {
    sleep(10);  // 等待文件打开
  }

  lseek(shared_fd, 0, SEEK_SET);  // 重置文件指针
  int bytes_read = read(shared_fd, read_buffer, sizeof(read_buffer));
  if (bytes_read > 0) {
    read_buffer[bytes_read] = '\0';  // 确保字符串终止符
    info(&logger, "Thread read: Read from file: %s\n", read_buffer);
  } else {
    info(&logger, "Thread read: Failed to read from file!\n");
  }
  pthread_exit(NULL);
}

void run() {
  pthread_t threads[num_threads];

  // 创建线程1: 打开文件并写入数据
  int rval = pthread_create(&threads[0], NULL, thread_func_open, NULL);
  info(&logger, "pthread_create return %d, thread_id: %d\n", rval, threads[0]);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 创建其他线程: 读取文件内容
  for (int i = 1; i < num_threads; i++) {
    rval = pthread_create(&threads[i], NULL, thread_func_read, NULL);
    info(&logger, "pthread_create return %d, thread_id: %d\n", rval,
         threads[i]);
    if (rval != 0) {
      cleanup();
      exit(TC_FAIL);
    }
  }

  // 等待所有线程完成
  for (int i = 0; i < num_threads; i++) {
    rval = pthread_join(threads[i], NULL);
    info(&logger, "pthread_join return %d\n", rval);
    if (rval != 0) {
      cleanup();
      exit(TC_FAIL);
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
