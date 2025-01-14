#pragma once

#include <sys/syscall.h>
#include <stdarg.h>
#include <io.h> // IWYU pragma: export

#define MAX_FILENAME_LEN 12 //<! only orange limit this

#define PRINT_BUF_LEN 1024

#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2

#define stdin STD_IN
#define stdout STD_OUT
#define stderr STD_ERR

#define EOF -1

int atoi(const char *nptr);

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

int vfprintf(int fd, const char *fmt, va_list ap);
int fprintf(int fd, const char *fmt, ...);
int printf(const char *fmt, ...);

int scanf(char *str, ...);
char getchar();
char *gets(char *str);
char fgetc(int fd);
char *fgets(char *str, int size, int fd);
