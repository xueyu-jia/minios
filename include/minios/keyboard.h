/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-15 09:40:51
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 18:22:03
 * @FilePath: /minios/os/include/kernel/keyboard.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef _ORANGES_KEYBOARD_H_
#define _ORANGES_KEYBOARD_H_
#include <minios/console.h>
#include <minios/type.h>
// #include <minios/keymap.h>

#define KB_inbS 320 /* size of keyboard input buffer */ /* FIXME */
#define MOUSE_inbS 3

typedef struct kb_inbuf {
    u8* p_head; /**< points to the next free slot */
    u8* p_tail; /**< points to the byte to be handled */
    int count;  /**< how many bytes to be handled in the buffer */
    u8 buf[KB_inbS];
} KB_INPUT;

#define MOUSE_UPDOWN_BOUND 15
typedef struct mouse_inbuf {
    int count;
    u8 buf[MOUSE_inbS];
} MOUSE_INPUT;

void keyboard_read(TTY* p_tty);
void init_kb();
#endif /* _ORANGES_KEYBOARD_H_ */
