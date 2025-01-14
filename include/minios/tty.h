#pragma once

#include <minios/console.h>
#include <klib/stdint.h>
#include <fs/fs.h>

#define TTY_inbS 256      /* tty input queue size */
#define TTY_OUT_BUF_LEN 2 /* tty output buffer size */

#define TTY_STATE_WAIT_ENTER 4
#define TTY_STATE_WAIT_SPACE 2
#define TTY_STATE_DISPLAY 1

typedef struct {
    u32 ibuf[TTY_inbS]; /* tty_t input buffer */
    u32* ibuf_head;     /* the next free slot */
    u32* ibuf_tail;     /* 缓冲区显示位置指针 */
    u32* ibuf_read;
    int ibuf_cnt; /* how many */
    int ibuf_read_cnt;
    int status;
    int mouse_left_button;
    int mouse_mid_button;
    int mouse_X;
    int mouse_Y;
    console_t* console;
} tty_t;

extern tty_t tty_table[NR_CONSOLES];

extern struct file_operations tty_file_ops;

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

void in_process(tty_t* tty, u32 key);
void init_screen(tty_t* tty);
void init_ttys();
void task_tty();
void wake_the_tty();
