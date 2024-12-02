#ifndef __TEST_COMMON__
#define __TEST_COMMON__

#include <const.h>
#include <errno.h>
#include <malloc.h>
#include <msg.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <type.h>
#include <ushm.h>

#define DIR_PATH_INEXISTE -1
#define DIR_PATH_REPEATED -2
#define DIR_FILE_OCCUPIYED -3

#define MAX_MSQ_NUM 4
#define MAX_MSG_IN_Q 1024
#define MAX_MSGBYTES (1 << 14)

#define MSG_SIZE 1024  // 暂时先用 1024......

#define TC_FAIL -1       // 测试失败
#define TC_PASS 0        // 测试成功
#define TC_UNRESOLVED 1  // 未处理

extern const char *LOG_LEVEL_LITERAL[];
extern const char *log_filename;

typedef enum {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
} logger_level;

// logger
typedef struct {
  char *log_filename;
  int logfd;
  char *test_name;
  logger_level level;
} logging;

typedef u32 uint32_t;

struct msgbuf {
  long mtype;
  char mtext[MSG_SIZE];
};

// TODO add cleanup function parameter

#define SAFE_FORK() safe_fork(__FILE__, __LINE__, NULL)

#define SAFE_OPEN(filename, oflags) \
  safe_open(__FILE__, __LINE__, NULL, (filename), (oflags))

#define SAFE_CLOSE(fd) safe_close(__FILE__, __LINE__, fd)

#define SAFE_WRITE(fd, buf, nbytes) \
  safe_write(__FILE__, __LINE__, fd, buf, nbytes)

#define SAFE_READ(fd, buf, nbytes) \
  safe_read(__FILE__, __LINE__, fd, buf, nbytes)

int safe_fork(const char *file, const int lineno, void (*cleanup_fn)(void));
int safe_open(const char *file, const int lineno, void (*cleanup_fn)(void),
              const char *filename, int oflags);
int safe_close(const char *file, const int lineno, int fd);
int safe_write(const char *file, const int lineno, int fd, const void *buf,
               int nbytes);
int safe_read(const char *file, const int lineno, int fd, void *buf,
              int nbytes);

int logger_init(logging *logger, const char *log_filename,
                const char *test_name, logger_level level);
int logger_close(logging *logger);
int debug(logging *logger, const char *fmt, ...);
int info(logging *logger, const char *fmt, ...);
int warning(logging *logger, const char *fmt, ...);
int error(logging *logger, const char *fmt, ...);
int fatal(logging *logger, const char *fmt, ...);

void cleanup();

#endif
