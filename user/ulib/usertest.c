#include "usertest.h"

const char *log_filename = "/alltestlog.txt";

const char *LOG_LEVEL_LITERAL[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
};

int safe_fork(const char *file, const int lineno, void (*cleanup_fn)(void)) {
  int pid = fork();
  if (pid < 0) {
    printf("%s:%d: fork() failed, return value %d\n", file, lineno, pid);
    if (cleanup_fn)
      (*cleanup_fn)();
    exit(-1);
  }
  return pid;
}

int safe_open(const char *file, const int lineno, void (*cleanup_fn)(void),
              const char *filename, int oflags) {
  int fd = open(filename, oflags, I_RWX);
  if (fd < 0) {
    printf("%s:%d: open(%s, %d) failed, return value %d\n", file, lineno,
           filename, oflags, fd);
    if (cleanup_fn) {
      (*cleanup_fn)();
    }
    exit(-1);
  }
  return fd;
}

int safe_close(const char *file, const int lineno, int fd) {
  if (fd < 0)
    return 0;

  int rval = close(fd);

  if (rval != 0) {
    printf("%s:%d: close(%d) failed, return value %d\n", file, lineno, fd,
           rval);
    exit(-1);
  }

  return rval;
}

int safe_write(const char *file, const int lineno, int fd, const void *buf,
               int nbytes) {
  int n = write(fd, buf, nbytes);
  if (n < 0) {
    printf("%s:%d: write(%d, %p, %d) failed, return value %d\n", file, lineno,
           fd, buf, nbytes, n);
    // if (cleanup_fn != NULL)
    // cleanup_fn();
    exit(-1);
  }
  // 只警告
  if (n < nbytes) {
    printf("%s:%d: warning: write(%d, %p, %d) return value %d, expected %d\n",
           file, lineno, fd, buf, nbytes, n, nbytes);
  }
  return n;

  /*
int rval;
const void *wbuf = buf;
int len = nbytes;

do {
  rval = write(fd, wbuf, len);
  if (rval < 0) {
    printf("%s: write(%d, %p, %d) failed, return value %d\n", testname, fd,
           buf, nbytes, rval);
    exit(-1);
  }

  wbuf += rval;
  len -= rval;
} while (len > 0);

return rval;
  */
}

int safe_read(const char *file, const int lineno, int fd, void *buf, int size) {
  int rval = read(fd, buf, size);
  if (rval < 0) {
    printf("%s:%s: read(%d, %p, %d) failed, return %d", file, lineno, fd, buf,
           size, rval);
    // if (cleanup_fn != NULL)
    // cleanup_fn();
    exit(-1);
  }
  return rval;
}

// 初始化一个 logger. 成功则返回 0，否则返回 -1。
int logger_init(logging *logger, const char *log_filename,
                const char *test_name, logger_level level) {
  int fd;
  int rval;

  if (logger == NULL) {
    printf("logger_init: ERROR: logger is NULL\n");
    return -1;
  }

  fd = open(log_filename, O_RDWR | O_APPEND);
  if (fd < 0) {
    // 文件不存在，尝试创建新文件
    fd = open(log_filename, O_RDWR | O_CREAT, I_RW);
    if (fd < 0) {
      printf("logger_init: ERROR: failed to open or create %s, return %d\n",
             log_filename, fd);
      return -1;
    }
  }

  if (level > LOG_ERROR || level < LOG_DEBUG) {
    printf("logger_init: ERROR: invalid logger_level\n");
    return -1;
  }

  logger->log_filename = log_filename;
  logger->logfd = fd;
  logger->test_name = test_name;
  logger->level = level;

  return 0;
}

int logger_close(logging *logger) {
  if (logger == NULL) {
    return 0;
  }

  if (logger->logfd >= 0) {
    close(logger->logfd);
  }
  memset(logger, 0, sizeof(logging));
}

// check logger. if logger is valid, return true, else false will be returned.
int check_logger(logging *logger) {
  return logger && logger->logfd >= 0 && logger->level >= LOG_DEBUG &&
         logger->level <= LOG_ERROR;
}

// real output function
int logger_write(logger_level current_level, logging *logger, const char *fmt,
                 va_list args) {
  int FMT_SIZE = 4096;
  int rval;
  int nbytes = 0;
  char real_fmt[FMT_SIZE];
  memset(real_fmt, 0, FMT_SIZE);
  char write_buf[FMT_SIZE];
  memset(write_buf, 0, FMT_SIZE);

  sprintf(real_fmt, "%s: %s: %s", logger->test_name,
          LOG_LEVEL_LITERAL[current_level], fmt);

  nbytes = vsprintf(write_buf, real_fmt, args);

  printf("%s", write_buf); // write to stdout

  // 这里分别使用 lseek 和 write 并不能保证原子性
  // 要保证原子地追加数据到文件末尾，需在 open 时使用 O_APPEND 模式。
  // MiniOS 尚未支持这一特性
  // update 2024.7 jiangfeng: VFS open O_APPEND added
  // rval = lseek(logger->logfd, 0, SEEK_END);
  // if (rval < 0) {
  // printf("logging: ERROR: lseek(%d, %d, %d) return %d\n", logger->logfd, 0,
  //       SEEK_END, rval);
  // return -1;
  //}
  write(logger->logfd, write_buf, nbytes); // write to log file
  return nbytes;
}

int logger_print(logger_level current_level, logging *logger, const char *fmt,
                 va_list args) {

  // printf("logging: pid: %d, tid: %d, logger: %d\n", get_pid(),
  // pthread_self(), logger);
  if (!check_logger(logger)) {
    printf("logging: ERROR: invalid logger\n");
    // printf(
    // "logging: logger: %d, fd: %d, level: %d, filename: %s, testname: %s\n",
    // logger, logger->logfd, logger->level, logger->log_filename,
    // logger->test_name);
    return -1;
  }

  // logger 的 log_level 大于 current_level，不输出
  if (current_level < logger->level) {
    return 0;
  }

  return logger_write(current_level, logger, fmt, args);
}

// 出错返回 -1。成功则返回写入的字节数
int debug(logging *logger, const char *fmt, ...) {
  va_list args = (va_list)((char *)(&fmt) + 4);
  return logger_print(LOG_DEBUG, logger, fmt, args);
}

// 出错返回 -1。成功则返回写入的字节数
int info(logging *logger, const char *fmt, ...) {
  va_list args = (va_list)((char *)(&fmt) + 4);
  return logger_print(LOG_INFO, logger, fmt, args);
}

// 出错返回 -1。成功则返回写入的字节数
int warning(logging *logger, const char *fmt, ...) {
  va_list args = (va_list)((char *)(&fmt) + 4);
  return logger_print(LOG_WARNING, logger, fmt, args);
}

// 出错返回 -1。成功则返回写入的字节数
int error(logging *logger, const char *fmt, ...) {
  va_list args = (va_list)((char *)(&fmt) + 4);
  return logger_print(LOG_ERROR, logger, fmt, args);
}
