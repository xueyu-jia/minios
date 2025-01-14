#pragma once

#include <minios/spinlock.h>

#define NR_CONSOLES 3

typedef struct {
    unsigned int crtc_start; /* set CRTC start addr reg */
    unsigned int orig;       /* start addr of the console */
    unsigned int con_size;   /* how many words does the console have */
    unsigned int cursor;
    int is_full;
    unsigned int current_line;
} console_t;

#define SCR_UP 1  /* scroll upward */
#define SCR_DN -1 /* scroll downward */

extern console_t console_table[];
extern int current_console;

extern int disp_pos;
extern spinlock_t video_mem_lock; // 用于 kprintf 调用与 tty_write 互斥，内核初始化完成后生效

void out_char(console_t* con, char ch);
void select_console(int nr_console);
void scroll_screen(console_t* con, int dir);
int is_current_console(console_t* con);
void reset_screen(console_t* con);

void kprintf(const char* fmt, ...);
