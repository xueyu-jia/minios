/********************************************
 *用户库函数声明		add by visual 2016.5.16
 *************************************************/
#ifndef _STDIO_H_   // added by mingxuan 2019-5-19
#define _STDIO_H_   // added by mingxuan 2019-5-19
#include <const.h>  //added by mingxuan 2019-5-19
#include <fcntl.h>
#include <syscall.h>
#include <type.h>  //added by mingxuan 2019-5-19

extern int tty;

/*syscall.asm*/
// int get_ticks();
// int get_pid();
// pthread_t  pthread_self();		//added by ZengHao & MaLinhan 21.12.23

/*  //deleted by mingxuan 2021-8-7
void* kmalloc(int size);
void* kmalloc_4k();
void* malloc(int size);
void* malloc_4k();
int free(void *arg);
int free_4k(void* AdddrLin);
*/

// int fork();
// int pthread_create(int *thread, void *attr, void *entry, void *arg);
// void pthread_exit(void *retval);
// int pthread_join(pthread_t pthread, void **retval);

// void udisp_int(int arg);
// void udisp_str(char* arg);

// added by xw
/* file system */
#define MAX_FILENAME_LEN 12  //? only orange limit this

#define PRINT_BUF_LEN 1024

/*printf.c*/
// added by mingxuan 2019-5-19
#define EOF -1
#define isspace(s) (s == ' ')

#define TOLOWER(x) ((x) | 0x20)
#define isxdigit(c)                                            \
  (('0' <= (c) && (c) <= '9') || ('a' <= (c) && (c) <= 'f') || \
   ('A' <= (c) && (c) <= 'F'))
#define isdigit(c) ('0' <= (c) && (c) <= '9')

int atoi(const char *nptr);

PUBLIC int vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC int sprintf(char *buf, const char *fmt, ...);
PUBLIC int fprintf(int fd, const char *fmt, ...);
PUBLIC int printf(const char *fmt, ...);
PUBLIC int scanf(char *str, ...);
char getchar();         // added by mingxuan 2019-5-23
char *gets(char *str);  // added by mingxuan 2019-5-23
char fgetc(int fd);
char *fgets(char *str, int size, int fd);
#endif  // added by mingxuan 2019-5-19
