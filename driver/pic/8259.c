#include <driver/pic/8259.h>
#include <minios/asm.h>
#include <minios/assert.h>
#include <klib/stddef.h>
#include <compiler.h>

void NAKED pic_io_wait() {
    asm volatile(
        "nop\n"
        "ret\n");
}

int pic_8259_init(int irq0_off, int irq8_off) {
    //! icw 1: init
    outb(PIC_M_PORT + PIC_CMD, PIC_CMD_ICW1_RESERVED | PIC_CMD_ICW1_ICW4);
    pic_io_wait();
    outb(PIC_S_PORT + PIC_CMD, PIC_CMD_ICW1_RESERVED | PIC_CMD_ICW1_ICW4);
    pic_io_wait();

    //! icw 2: set int vector offset
    outb(PIC_M_PORT + PIC_DATA, irq0_off);
    pic_io_wait();
    outb(PIC_S_PORT + PIC_DATA, irq8_off);
    pic_io_wait();

    //! icw 3: set cascade mode, cascade at irq 2
    outb(PIC_M_PORT + PIC_DATA, BIT(2));
    pic_io_wait();
    outb(PIC_S_PORT + PIC_DATA, 2);
    pic_io_wait();

    //! icw 4: set ctl word
    outb(PIC_M_PORT + PIC_DATA, PIC_DATA_ICW4_8086);
    pic_io_wait();
    outb(PIC_S_PORT + PIC_DATA, PIC_DATA_ICW4_8086);
    pic_io_wait();

    pic_disable();

    return 0;
}

void pic_disable() {
    pic_set_mask(0xffff);
}

uint16_t pic_get_mask() {
    return inb(PIC_M_PORT + PIC_DATA) | (inb(PIC_S_PORT + PIC_DATA) << 8);
}

void pic_set_mask(uint16_t mask) {
    outb(PIC_M_PORT + PIC_DATA, (mask >> 0) & 0xff);
    outb(PIC_S_PORT + PIC_DATA, (mask >> 8) & 0xff);
}

void pic_set_irq_mask(int irq, bool on) {
    assert(irq >= 0 && irq < 16);
    const uint8_t mask_bit = BIT(irq & 0x7);
    const int port = irq < 8 ? PIC_M_PORT : PIC_S_PORT;
    const int mask = inb(port + PIC_DATA);
    outb(port + PIC_DATA, on ? mask | mask_bit : mask & ~mask_bit);
}

bool pic_is_irq_masked(int irq) {
    assert(irq >= 0 && irq < 16);
    return !!(inb((irq < 8 ? PIC_M_PORT : PIC_S_PORT) + PIC_DATA) & BIT(irq & 0x7));
}

void pic_send_eoi(int irq) {
    assert(irq >= 0 && irq < 16);
    if (irq >= 8) { outb(PIC_S_PORT + PIC_CMD, PIC_CMD_EOI); }
    outb(PIC_M_PORT + PIC_CMD, PIC_CMD_EOI);
}

uint16_t pic_get_irr() {
    outb(PIC_M_PORT + PIC_CMD, PIC_CMD_READ_IRR);
    outb(PIC_S_PORT + PIC_CMD, PIC_CMD_READ_IRR);
    return inb(PIC_M_PORT + PIC_CMD) | (inb(PIC_S_PORT + PIC_CMD) << 8);
}

uint16_t pic_get_isr() {
    outb(PIC_M_PORT + PIC_CMD, PIC_CMD_READ_ISR);
    outb(PIC_S_PORT + PIC_CMD, PIC_CMD_READ_ISR);
    return inb(PIC_M_PORT + PIC_CMD) | (inb(PIC_S_PORT + PIC_CMD) << 8);
}
