#pragma once

#include <klib/spinlock.h>

/* CONSOLE */
typedef struct s_console {
    unsigned int crtc_start; /* set CRTC start addr reg */
    unsigned int orig;       /* start addr of the console */
    unsigned int con_size;   /* how many words does the console have */
    unsigned int cursor;
    int is_full;
    unsigned int current_line;
} CONSOLE;

#define SCR_UP 1  /* scroll upward */
#define SCR_DN -1 /* scroll downward */

// #define SCR_SIZE		(80 * 25)
// #define SCR_WIDTH		 80

// #define DEFAULT_CHAR_COLOR	(MAKE_COLOR(BLACK, WHITE) | BRIGHT)
// #define GRAY_CHAR		(MAKE_COLOR(BLACK, BLACK) | BRIGHT)
// #define RED_CHAR		(MAKE_COLOR(BLUE, RED) | BRIGHT)
extern int disp_pos;
extern CONSOLE console_table[];
extern SPIN_LOCK video_mem_lock; // 用于 disp调用与tty_write互斥,内核初始化完成后生效

void kprintf(const char* fmt, ...);

typedef struct s_tty TTY;
void init_screen(TTY* tty);
void out_char(CONSOLE* con, char ch);
void select_console(int nr_console);
void scroll_screen(CONSOLE* con, int dir);
int is_current_console(CONSOLE* con);
void reset_screen(CONSOLE* con);
