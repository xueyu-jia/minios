#pragma once

#include <klib/stdint.h>
#include <minios/tty.h>

#define KB_inbS 320
#define MOUSE_inbS 3
#define MOUSE_UPDOWN_BOUND 15

typedef struct kb_inbuf {
    u8* p_head; //<! points to the next free slot
    u8* p_tail; //<! points to the byte to be handled
    int count;  //<! how many bytes to be handled in the buffer
    u8 buf[KB_inbS];
} kb_inbuf_t;

typedef struct mouse_inbuf {
    int count;
    u8 buf[MOUSE_inbS];
} mouse_inbuf_t;

void init_kb();
void keyboard_read(tty_t* tty);
