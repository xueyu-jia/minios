#include <minios/tty.h>
#include <minios/dev.h>
#include <minios/keyboard.h>
#include <minios/keymap.h>
#include <minios/semaphore.h>
#include <minios/vga.h>
#include <compiler.h>
#include <stdbool.h>

tty_t tty_table[NR_CONSOLES];

int current_console; // 当前显示在屏幕上的console
MAYBE_UNUSED semaphore_t tty_empty;
static semaphore_t tty_full;
static semaphore_t tty_buf_read;

static void put_key(tty_t* tty, u32 key) {
    if (tty->ibuf_cnt < TTY_inbS) {
        *(tty->ibuf_head) = key;
        tty->ibuf_head++;
        if (tty->ibuf_head == tty->ibuf + TTY_inbS) tty->ibuf_head = tty->ibuf;
        tty->ibuf_cnt++;
        tty->ibuf_read_cnt++;
        // ksem_post(&tty_full, 1);
    }
}

void wake_the_tty() { // 让tty的锁永远小于等于1
    if (tty_full.value < 1) ksem_post(&tty_full, 1);
}

void in_process(tty_t* tty, u32 key) {
    int real_line = tty->console->orig / SCR_WIDTH;
    UNUSED(real_line);

    if (!(key & FLAG_EXT)) {
        put_key(tty, key);
    } else {
        const int raw_code = key & MASK_RAW;
        switch (raw_code) {
            case ENTER: {
                put_key(tty, '\n');
                tty->status = tty->status & 0b11;
                ksem_post(&tty_buf_read, 1);
            } break;
            case BACKSPACE: {
                put_key(tty, '\b');
            } break;
            case UP: {
                scroll_screen(tty->console, SCR_UP);
            } break;
            case DOWN: {
                scroll_screen(tty->console, SCR_DN);
            } break;
            case F1:
                FALLTHROUGH;
            case F2:
                FALLTHROUGH;
            case F3:
                FALLTHROUGH;
            case F4:
                FALLTHROUGH;
            case F5:
                FALLTHROUGH;
            case F6:
                FALLTHROUGH;
            case F7:
                FALLTHROUGH;
            case F8:
                FALLTHROUGH;
            case F9:
                FALLTHROUGH;
            case F10:
                FALLTHROUGH;
            case F11:
                FALLTHROUGH;
            case F12: {
                select_console(raw_code - F1);
            } break;
        }
    }
}

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

static void init_tty(tty_t* tty) {
    tty->ibuf_read_cnt = tty->ibuf_cnt = 0;
    tty->status = TTY_STATE_DISPLAY;
    tty->ibuf_read = tty->ibuf_head = tty->ibuf_tail = tty->ibuf;
    int det = tty - tty_table;
    tty->console = console_table + det;

    tty->mouse_left_button = 0;
    tty->mouse_mid_button = 0;
    tty->mouse_X = 0;
    tty->mouse_Y = 0;
    init_screen(tty);
}

static void tty_mouse(tty_t* tty) {
    if (is_current_console(tty->console)) {
        int real_line = tty->console->orig / SCR_WIDTH;
        UNUSED(real_line);
        if (tty->mouse_left_button) {
            if (tty->mouse_Y > MOUSE_UPDOWN_BOUND) { // 按住鼠标左键向上滚动
                scroll_screen(tty->console, SCR_UP);
                tty->mouse_Y = 0;
            } else if (tty->mouse_Y < -MOUSE_UPDOWN_BOUND) { // 按住鼠标左键向下滚动
                scroll_screen(tty->console, SCR_DN);
                tty->mouse_Y = 0;
            }
        }
        if (tty->mouse_mid_button) { // 点击中键复原
            reset_screen(tty->console);
            tty->mouse_Y = 0;
        }
    }
}

static void tty_dev_read(tty_t* tty) {
    if (is_current_console(tty->console)) { keyboard_read(tty); }
}

static void tty_dev_write(tty_t* tty) {
    if (tty->ibuf_cnt) {
        char ch = *(tty->ibuf_tail);
        tty->ibuf_tail++;
        if (tty->ibuf_tail == tty->ibuf + TTY_inbS) { tty->ibuf_tail = tty->ibuf; }
        tty->ibuf_cnt--;

        if (ch == '\b') {
            if (tty->ibuf_read_cnt == 1) {
                tty->ibuf_read_cnt--;
                tty->ibuf_head--;
                tty->ibuf_tail--;
                // tty->ibuf_read++;
                return;
            } else {
                tty->ibuf_read_cnt -= 2;
                // tty->ibuf_read++;
                // tty->ibuf_read++;
                if (tty->ibuf_head == tty->ibuf) {
                    tty->ibuf_head = tty->ibuf_tail = &tty->ibuf[256 - 2];
                } else {
                    tty->ibuf_head--;
                    tty->ibuf_tail--;
                    tty->ibuf_head--;
                    tty->ibuf_tail--;
                }
            }
        }
        spinlock_acquire(&video_mem_lock);
        out_char(tty->console, ch);
        spinlock_release(&video_mem_lock);
    }
}

void init_ttys() {
    tty_t* tty;
    for (tty = TTY_FIRST; tty < TTY_END; ++tty) { init_tty(tty); }
    tty = TTY_FIRST;

    select_console(0);
    ksem_init(&tty_full, 0);
}

void task_tty() {
    while (true) {
        ksem_wait(&tty_full, 1);
        for (tty_t* tty = TTY_FIRST; tty < TTY_END; ++tty) {
            do {
                tty_mouse(tty);     /* tty判断鼠标操作 */
                tty_dev_read(tty);  /* 从键盘输入缓冲区读到这个tty自己的缓冲区 */
                tty_dev_write(tty); /* 把tty缓存区的数据写到这个tty占有的显存 */

            } while (tty->ibuf_cnt);
        }
        // yield();
    }
}

/*****************************************************************************
 *                                tty_write
 ****************************************************************************

 *  当fd=STD_OUT时，write()系统调用转发到此函数 不走task_tty
 *****************************************************************************/
void tty_write(tty_t* tty, const char* buf, int len) {
    spinlock_acquire(&video_mem_lock);
    while (--len >= 0) out_char(tty->console, *buf++);
    spinlock_release(&video_mem_lock);
}

/*****************************************************************************
 *                                tty_read
 ****************************************************************************

 *  当fd=STD_IN时，read()系统调用转发到此函数 不走task_tty
 *****************************************************************************/
int tty_read(tty_t* tty, char* buf, int len) {
    int i = 0;
    if (!tty->ibuf_read_cnt) {
        tty->status |= TTY_STATE_WAIT_ENTER;
        ksem_wait(&tty_buf_read, 1);
    }

    while ((tty->status & TTY_STATE_WAIT_ENTER)); // 等待回车按下

    // if((tty->ibuf_head-tty->ibuf) >= tty->ibuf_cnt ){
    //     start = tty->ibuf_head - tty->ibuf_cnt;
    // }else{
    //     start = tty->ibuf_head + (TTY_inbS - tty->ibuf_cnt);
    // }

    while (tty->ibuf_read_cnt && i < len) {
        buf[i] = *tty->ibuf_read;
        tty->ibuf_read++;
        if (tty->ibuf_read == tty->ibuf + TTY_inbS) { tty->ibuf_read = tty->ibuf; }
        tty->ibuf_read_cnt--;
        i++;
    }

    return i;
}

int tty_file_write(struct file_desc* file, unsigned int count, const char* buf) {
    int dev = file->fd_dentry->d_inode->i_b_cdev;
    int nr_tty = MINOR(dev);
    if (MAJOR(dev) != DEV_CHAR_TTY) { kprintf("Error: MAJOR(dev) != DEV_CHAR_TTY\n"); }
    tty_write(&tty_table[nr_tty], buf, count);
    return count;
}

int tty_file_read(struct file_desc* file, unsigned int count, char* buf) {
    int dev = file->fd_dentry->d_inode->i_b_cdev;
    int nr_tty = MINOR(dev);
    if (MAJOR(dev) != DEV_CHAR_TTY) { kprintf("Error: MAJOR(dev) != DEV_CHAR_TTY\n"); }
    return tty_read(&tty_table[nr_tty], buf, count);
}

struct file_operations tty_file_ops = {
    .read = tty_file_read,
    .write = tty_file_write,
};
