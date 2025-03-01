#pragma once

#include <uapi/minios/interrupt.h>
#include <minios/asm.h>
#include <minios/types.h>
#include <minios/regs.h>
#include <klib/stdint.h>
#include <stdbool.h>

//! hardware interrupts
#define NR_IRQ 16
#define INT_VECTOR_IRQ0 0x20
#define INT_VECTOR_IRQ8 0x28

#define CLOCK_IRQ 0          //<! clock
#define KEYBOARD_IRQ 1       //<! keyboard
#define CASCADE_IRQ 2        //<! cascade enable for 2nd AT controller
#define ETHER_IRQ 3          //<! default ethernet interrupt vector
#define SECONDARY_IRQ 3      //<! RS232 interrupt vector for port 2
#define RS232_IRQ 4          //<! RS232 interrupt vector for port 1
#define XT_WINI_IRQ 5        //<! xt winchester
#define FLOPPY_IRQ 6         //<! floppy disk
#define PRINTER_IRQ 7        //<! printer
#define REALTIME_CLOCK_IRQ 8 //<! realtime clock
#define REDIRECT_IRQ 9       //<! irq 2 redirected
#define MOUSE_IRQ 12         //<! mouse (or else)
#define FPU_IRQ 13           //<! fpu exception
#define AT_WINI_IRQ 14       //<! AT winchester disk

//! software interrupts
#define INT_VECTOR_DIVIDE 0x00
#define INT_VECTOR_DEBUG 0x01
#define INT_VECTOR_NMI 0x02
#define INT_VECTOR_BREAKPOINT 0x03
#define INT_VECTOR_OVERFLOW 0x04
#define INT_VECTOR_BOUNDS 0x05
#define INT_VECTOR_INVAL_OP 0x06
#define INT_VECTOR_DEVICE_NOT 0x07
#define INT_VECTOR_DOUBLE_FAULT 0x08
#define INT_VECTOR_COPROC_SEG 0x09
#define INT_VECTOR_INVAL_TSS 0x0a
#define INT_VECTOR_SEG_NOT 0x0b
#define INT_VECTOR_STACK_FAULT 0x0c
#define INT_VECTOR_PROTECTION 0x0d
#define INT_VECTOR_PAGE_FAULT 0x0e
#define INV_VECTOR_FP_EXCEPTION 0x10

#define disable_int_begin()                                      \
    {                                                            \
        int __scoped_interrupt_flag = read_eflags() & EFLAGS_IF; \
        if (__scoped_interrupt_flag) { disable_int(); }

#define disable_int_end()                          \
    if (__scoped_interrupt_flag) { enable_int(); } \
    }

extern irq_handler_t irq_table[NR_IRQ];

bool is_irq_masked(int irq);
void enable_irq(int irq);
void disable_irq(int irq);
void spurious_irq(int irq);
void put_irq_handler(int irq, irq_handler_t handler);

void init_interrupt_controller();

void exception_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void general_protection_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void sys_call();
