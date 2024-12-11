/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-18 15:32:07
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 22:08:23
 * @FilePath: /minios/os/kernel/vga.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include <kernel/vga.h>
// #include <unios/layout.h>
#include <kernel/const.h>
#include <kernel/x86.h>
#include <klib/string.h>

// extern int disp_pos;

PRIVATE void vga_wait_for_vrs() {
    // while (!(inb(CRTC_ADDR_REG) & (1 << 3))) {}
}

void vga_set_video_start_addr(u32 addr) {
    disable_int();
    outb(CRTC_ADDR_REG, START_ADDR_H);
    outb(CRTC_DATA_REG, (addr >> 8) & 0xff);
    outb(CRTC_ADDR_REG, START_ADDR_L);
    outb(CRTC_DATA_REG, (addr >> 0) & 0xff);
    vga_wait_for_vrs();
    enable_int();
}

void vga_enable_cursor(u8 cur_start, u8 cur_end) {
    disable_int();
    outb(CRTC_ADDR_REG, 0x0a);
    outb(CRTC_DATA_REG, (inb(CRTC_DATA_REG) & 0xc0) | cur_start);
    outb(CRTC_ADDR_REG, 0x0b);
    outb(CRTC_DATA_REG, (inb(CRTC_DATA_REG) & 0xe0) | cur_end);
    vga_wait_for_vrs();
    enable_int();
}

void vga_disable_cursor() {
    disable_int();
    outb(CRTC_ADDR_REG, 0x0a);
    outb(CRTC_DATA_REG, 0x20);
    vga_wait_for_vrs();
    enable_int();
}

void vga_set_cursor(u32 pos) {
    disable_int();
    outb(CRTC_ADDR_REG, CURSOR_H);
    outb(CRTC_DATA_REG, (pos >> 8) & 0xff);
    outb(CRTC_ADDR_REG, CURSOR_L);
    outb(CRTC_DATA_REG, (pos >> 0) & 0xff);
    vga_wait_for_vrs();
    enable_int();
}

void vga_clear_screen() {
    u16 *ptr = (u16 *)K_PHY2LIN(V_MEM_BASE);
    u16 *end = ptr + SCR_WIDTH * SCR_HEIGHT;
    while (ptr != end) { *ptr++ = BLANK; }
    // disp_pos = 0;
    vga_set_cursor(0);
}

void vga_flush_blankline(int line_no) {
    u32 *dst = (u32 *)K_PHY2LIN(V_MEM_BASE + line_no * SCR_WIDTH * 2);
    for (u32 i = 0; i < SCR_WIDTH * sizeof(u16) / sizeof(u32); ++i) {
        *dst++ = (BLANK << 16) | BLANK;
    }
}

void vga_put_raw(u32 pos, u16 dat) {
    *(u16 *)K_PHY2LIN(V_MEM_BASE + pos) = dat;
}

u32 vga_write_char(char ch, u16 color, u32 _disp_pos) {
    if (ch == '\n') {
        _disp_pos = (_disp_pos / SCR_WIDTH + 1) * SCR_WIDTH;
    } else {
        vga_put_raw(_disp_pos * 2, (color << 8) | (u16)ch);
        (_disp_pos)++;
    }
    // set_cursor(_disp_pos);
    return _disp_pos;
}

u32 vga_write_str(const char *str, u32 _disp_pos) {
    for (char *s = (char *)str; *s; ++s) {
        _disp_pos = vga_write_char(*s, WHITE, _disp_pos);
    }
    return _disp_pos;
}

u32 vga_write_colored_str(const char *str, u8 color, u32 _disp_pos) {
    for (char *s = (char *)str; *s; ++s) {
        _disp_pos = vga_write_char(*s, color, _disp_pos);
    }
    return _disp_pos;
}

void vga_copy(u32 dst, u32 src, size_t size) {
    memcpy((void *)K_PHY2LIN(V_MEM_BASE + (dst << 1)),
           (const void *)K_PHY2LIN(V_MEM_BASE + (src << 1)), size << 1);
}
