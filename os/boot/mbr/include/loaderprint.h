/*
 * @Author: Yuanhong.Yu 
 * @Date: 2023-01-04 16:42:43 
 * @Last Modified by:   Yuanhong.Yu 
 * @Last Modified time: 2023-01-04 16:42:43 
 */
//ported by sundong 2023.3.26

#ifndef _LOADERPRINT_H_
#define _LOADERPRINT_H_

#include "type.h"
#include "paging.h"
#include "elf.h"
#define TERMINAL_COLUMN	80
#define TERMINAL_ROW	25
#define VIDEO_MEM_START 0xB8000

#define TERMINAL_POS(row, column) ((u16)(row) * TERMINAL_COLUMN + (column))
/*
 * 终端默认色，黑底白字
 */
#define DEFAULT_COLOR 0x0f00

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)

#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define va_end(ap) __builtin_va_end(ap)

/*
 * 这个函数将content数据（2字节）写到终端第disp_pos个字符
 * 第0个字符在0行0列，第1个字符在0行1列，第80个字符在1行0列，以此类推
 * 用的是内联汇编，等价于mov word [gs:disp_pos * 2], content
 */
/*
 * 清屏
 */
extern int video_row;
extern int video_col;
void clear_screen();
//简单的print
void lprintf(char *s,...);
//打印内存信息
void print_mem(struct ARDStruct* adr);
void print_elf(struct Proghdr *ph);
#endif