#include <minios/console.h>
#include <minios/tty.h>
#include <minios/vga.h>
#include <minios/interrupt.h>
#include <minios/layout.h>
#include <minios/uart.h>
#include <minios/spinlock.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <fmt.h>

int disp_pos;
console_t console_table[NR_CONSOLES];
spinlock_t video_mem_lock; // 用于 disp调用与tty_write互斥,内核初始化完成后生效

static void flush(console_t* con);
static void w_copy(unsigned int dst, const unsigned int src, int size);
static void clear_screen(int pos, int len);

/*****************************************************************************
 *                                init_screen
 *****************************************************************************/
/**
 * Initialize the console of a certain tty.
 *
 * @param tty  Whose console is to be initialized.
 *****************************************************************************/
void init_screen(tty_t* tty) {
    int nr_tty = tty - tty_table;

    tty->console = console_table + nr_tty;

    /*
     * NOTE:
     *   variables related to `position' and `size' below are
     *   in WORDs, but not in BYTEs.
     */
    int v_mem_size = V_MEM_SIZE >> 1; /* size of Video Memory */
    int size_per_con = (v_mem_size / NR_CONSOLES) / 80 * 80;
    tty->console->orig = nr_tty * size_per_con;
    tty->console->con_size = size_per_con / SCR_WIDTH * SCR_WIDTH;
    tty->console->cursor = tty->console->crtc_start = tty->console->orig;
    tty->console->is_full = 0;
    tty->console->current_line = 0;

    if (nr_tty == 0) { tty->console->cursor = disp_pos / 2; }
    const char prompt[] = "[TTY #?]\n";

    const char* p = prompt;
    for (; *p; ++p) {
        // kprintf("a");
        // tty->console->cursor = disp_pos / 2;
        out_char(tty->console, *p == '?' ? nr_tty + '0' : *p);
    }

    vga_set_cursor(tty->console->cursor);
}

/*****************************************************************************
 *                                out_char
 *****************************************************************************/
/**
 * Print a char in a certain console.
 *
 * @param con  The console to which the char is printed.
 * @param ch   The char to print.
 *****************************************************************************/

void out_char(console_t* con, char ch) {
    // spinlock_acquire(&video_mem_lock);
    disable_int_begin();

    // u8* pch = (u8*)(V_MEM_BASE + con->cursor * 2);
    // unsigned int pos = V_MEM_BASE+con->cursor*2;

    // assert(con->cursor - con->orig < con->con_size);

    /*
     * calculate the coordinate of cursor in current console (not in
     * current screen)
     */
    int cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
    int cursor_y = (con->cursor - con->orig) / SCR_WIDTH;

    switch (ch) {
        case '\n':
            clear_screen(con->cursor, SCR_WIDTH - cursor_x);
            con->cursor = con->orig + SCR_WIDTH * (cursor_y + 1);
            break;
        case '\b':
            if (con->cursor > con->orig) {
                con->cursor--;
                //*(pch - 2) = ' ';
                //*(pch - 1) = DEFAULT_CHAR_COLOR;
                // disp_pos = con->cursor*2;
                // write_char(' ', con->cursor*2);
                vga_write_char(' ', WHITE_CHAR, con->cursor);
            }
            break;
        default:
            //*pch++ = ch;
            //*pch++ = DEFAULT_CHAR_COLOR;
            // disp_pos = con->cursor*2;
            // write_char(ch, con->cursor*2);
            con->cursor = vga_write_char(ch, WHITE_CHAR, con->cursor);

            break;
    }

    cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
    cursor_y = (con->cursor - con->orig) / SCR_WIDTH;
    if (con->cursor - con->orig >= con->con_size) {
        int cp_orig = con->orig + (cursor_y + 1) * SCR_WIDTH - SCR_SIZE;
        w_copy(con->orig, cp_orig, SCR_SIZE - SCR_WIDTH);
        con->crtc_start = con->orig;
        con->cursor = con->orig + (SCR_SIZE - SCR_WIDTH) + cursor_x;
        clear_screen(con->cursor, SCR_WIDTH - cursor_x);
        if (!con->is_full) con->is_full = 1;
    }

    // assert(con->cursor - con->orig < con->con_size);

    while (con->cursor >= con->crtc_start + SCR_SIZE || con->cursor < con->crtc_start) {
        scroll_screen(con, SCR_UP);

        clear_screen(con->cursor, SCR_WIDTH - cursor_x);
    }
    // if(con == console_table){
    // 	disp_pos = con->cursor*2;
    // }
    flush(con);

    disable_int_end();
    // spinlock_release(&video_mem_lock);
}

/*****************************************************************************
 *                                clear_screen
 *****************************************************************************/
/**
 * Write whitespaces to the screen.
 *
 * @param pos  Write from here.
 * @param len  How many whitespaces will be written.
 *****************************************************************************/
static void clear_screen(int pos, int len) {
    u8* pch = (u8*)(K_PHY2LIN(V_MEM_BASE) + pos * 2);
    while (--len >= 0) {
        *pch++ = ' ';
        *pch++ = DEFAULT_CHAR_COLOR;
    }
}

/*****************************************************************************
 *                            is_current_console
 *****************************************************************************/
/**
 * Uses `nr_current_console' to determine if a console is the current one.
 *
 * @param con   Ptr to console.
 *
 * @return   TRUE if con is the current console.
 *****************************************************************************/
int is_current_console(console_t* con) {
    return (con == &console_table[current_console]);
}

/*****************************************************************************
 *                                select_console
 *****************************************************************************/
/**
 * Select a console as the current.
 *
 * @param nr_console   Console nr, range in [0, NR_CONSOLES-1].
 *****************************************************************************/
void select_console(int nr_console) {
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) return;

    flush(&console_table[current_console = nr_console]);
}

/*****************************************************************************
 *                                scroll_screen
 *****************************************************************************/
/**
 * Scroll the screen.
 *
 * Note that scrolling UP means the content of the screen will go upwards, so
 * that the user can see lines below the bottom. Similarly scrolling DOWN means
 * the content of the screen will go downwards so that the user can see lines
 * above the top.
 *
 * When there is no line below the bottom of the screen, scrolling UP takes no
 * effects; when there is no line above the top of the screen, scrolling DOWN
 * takes no effects.
 *
 * @param con   The console whose screen is to be scrolled.
 * @param dir   SCR_UP : scroll the screen upwards;
 *              SCR_DN : scroll the screen downwards
 *****************************************************************************/
void scroll_screen(console_t* con, int dir) {
    /*
     * variables below are all in-console-offsets (based on con->orig)
     */
    u32 oldest;  /* addr of the oldest available line in the console */
    u32 newest;  /* .... .. ... latest ......... .... .. ... ....... */
    u32 scr_top; /* position of the top of current screen */

    newest = (con->cursor - con->orig) / SCR_WIDTH * SCR_WIDTH;
    oldest = con->is_full ? (newest + SCR_WIDTH) % con->con_size : 0;
    scr_top = con->crtc_start - con->orig;

    if (dir == SCR_DN) {
        if (!con->is_full && scr_top > 0) {
            con->crtc_start -= SCR_WIDTH;
        } else if (con->is_full && scr_top != oldest) {
            if (con->cursor - con->orig >= con->con_size - SCR_SIZE) {
                if (con->crtc_start != con->orig) con->crtc_start -= SCR_WIDTH;
            } else if (con->crtc_start == con->orig) {
                scr_top = con->con_size - SCR_SIZE;
                con->crtc_start = con->orig + scr_top;
            } else {
                con->crtc_start -= SCR_WIDTH;
            }
        }
    } else if (dir == SCR_UP) {
        if (!con->is_full && newest >= scr_top + SCR_SIZE) {
            con->crtc_start += SCR_WIDTH;
        } else if (con->is_full && scr_top + SCR_SIZE - SCR_WIDTH != newest) {
            if (scr_top + SCR_SIZE == con->con_size)
                con->crtc_start = con->orig;
            else
                con->crtc_start += SCR_WIDTH;
        }
    } else {
        // assert(dir == SCR_DN || dir == SCR_UP);
    }

    flush(con);
}

void reset_screen(console_t* con) {
    int latest_start = (con->cursor - con->orig) / SCR_WIDTH * SCR_WIDTH + SCR_WIDTH - SCR_SIZE;
    if (latest_start < 0) { latest_start = 0; }
    con->crtc_start = con->orig + latest_start;
    flush(con);
}

/*****************************************************************************
 *                                flush
 *****************************************************************************/
/**
 * Set the cursor and starting address of a console by writing the
 * CRT Controller Registers.
 *
 * @param con  The console to be set.
 *****************************************************************************/
static void flush(console_t* con) {
    if (is_current_console(con)) {
        vga_set_cursor(con->cursor);
        vga_set_video_start_addr(con->crtc_start);
    }
}

/*****************************************************************************
 *                                w_copy
 *****************************************************************************/
/**
 * Copy data in WORDS.
 *
 * Note that the addresses of dst and src are not pointers, but integers, 'coz
 * in most cases we pass integers into it as parameters.
 *
 * @param dst   Addr of destination.
 * @param src   Addr of source.
 * @param size  How many words will be copied.
 *****************************************************************************/
static void w_copy(unsigned int dst, const unsigned int src, int size) {
    memcpy((void*)(K_PHY2LIN(V_MEM_BASE) + (dst << 1)), (void*)(K_PHY2LIN(V_MEM_BASE) + (src << 1)),
           size << 1);
}

static inline bool do_putch(char ch) {
    const bool accepted = isprint(ch) || ch == '\n' || ch == '\t';
    if (accepted) { serial_write(ch); }
    return accepted;
}

static char* putch_handler(char* buf, void* user, int len) {
    for (int i = 0; i < len; ++i) {
        if (do_putch(buf[i])) { ++*(int*)user; }
    }
    return buf;
}

static int kvprintf(const char* fmt, va_list ap) {
    int cnt = 0;
    char buf[64] = {};
    strfmt_handler_t handler = {
        .callback = putch_handler,
        .user = &cnt,
    };
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);
    return cnt;
}

void kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
