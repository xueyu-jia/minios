/*
 * 测试 32 个信号是否都能够正常接收
 *
 */

#include "usertest.h"

const char *test_name = "signal01";
const char *syscall_name = "signal";

logging logger;

// 记录接收到的信号。每位表示一个信号
static volatile uint32_t recv_sig = 0;

void handler(int sig, uint32_t arg) {
  recv_sig |= 1 << sig;
  if (sig != arg) {
    info(&logger, "sig: %d != arg: %d\n", sig, arg);
  }
}

void int2bin(uint32_t num, char bin[32]) {
  for (int i = 0; i < 32; i++) {
    bin[i] = num & 0x01 ? '1' : '0';
    num >>= 1;
  }
}

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int parent_pid = get_pid();
  int pid = SAFE_FORK();
  if (pid == 0) {
    // child process
    sleep(100); // wait for parent process register signal
    // send signal to parent process
    for (int sig = 0; sig < 32; sig++) {
      int rval = kill(parent_pid, sig, sig);
      if (rval < 0) {
        info(&logger, "not register\n");
        cleanup();
        exit(-1);
      }
    }
    exit(0);
  }

  // parent process
  // 注册所有信号
  for (int sig = 0; sig < 32; sig++) {
    int rval = signal(sig, handler);
    if (rval == -1) {
      info(&logger, "failed to register signal %d\n", sig);
      cleanup();
      exit(-1);
    }
  }
  info(&logger, "register signal %d to %d\n", 0, 31);
  // 等待所有信号处理完毕
  sleep(1000);

  // convert int to string
  char recv_bin[33];
  char expected_bin[33];
  memset(recv_bin, 0, 33);
  memset(expected_bin, 0, 33);
  int2bin(recv_sig, recv_bin);
  int2bin(0xFFFFFFFF, expected_bin);

  info(&logger, "signal received %s, expected %s\n", recv_bin, expected_bin);
  if (recv_sig != 0xFFFFFFFF) {
    cleanup();
    exit(-1);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}