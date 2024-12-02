#pragma once

#include <kernel/console.h>
#include <kernel/regs.h>
#include <kernel/type.h>
#include <kernel/x86-asm.h>

//: 8259A interrupt controller ports
//: <Master> I/O port for interrupt controller
#define INT_M_CTL 0x20
//: <Master> setting bits in this port disables ints
#define INT_M_CTLMASK 0x21
//: <Slave> I/O port for second interrupt controller
#define INT_S_CTL 0xA0
//: <Slave> setting bits in this port disables ints
#define INT_S_CTLMASK 0xA1

//: 8253/8254 PIT (Programmable Interval Timer)
//: I/O port for timer channel 0
#define TIMER0 0x40
//: I/O port for timer mode control
#define TIMER_MODE 0x43
//: 00-11-010-0 : Counter0 - LSB then MSB - rate generator - binary
#define RATE_GENERATOR 0x34
//: clock frequency for timer in PC and AT
#define TIMER_FREQ 1193182L
//: clock freq (software settable on IBM-PC)
#define HZ 100

//: hardware interrupts
#define NR_IRQ 16
#define CLOCK_IRQ 0           //<! clock
#define KEYBOARD_IRQ 1        //<! keyboard
#define CASCADE_IRQ 2         //<! cascade enable for 2nd AT controller
#define ETHER_IRQ 3           //<! default ethernet interrupt vector
#define SECONDARY_IRQ 3       //<! RS232 interrupt vector for port 2
#define RS232_IRQ 4           //<! RS232 interrupt vector for port 1
#define XT_WINI_IRQ 5         //<! xt winchester
#define FLOPPY_IRQ 6          //<! floppy disk
#define PRINTER_IRQ 7         //<! printer
#define REALTIME_CLOCK_IRQ 8  //<! realtime clock
#define REDIRECT_IRQ 9        //<! irq 2 redirected
#define MOUSE_IRQ 12          //<! mouse (or else)
#define FPU_IRQ 13            //<! fpu exception
#define AT_WINI_IRQ 14        //<! AT winchester disk

/* 中断向量 */
#define INT_VECTOR_IRQ0 0x20
#define INT_VECTOR_IRQ8 0x28

#define INT_VECTOR_DIVIDE 0x00
#define INT_VECTOR_DEBUG 0x01
#define INT_VECTOR_NMI 0x02
#define INT_VECTOR_BREAKPOINT 0x03
#define INT_VECTOR_OVERFLOW 0x04
#define INT_VECTOR_BOUNDS 0x05
#define INT_VECTOR_INVAL_OP 0x06
#define INT_VECTOR_COPROC_NOT 0x07
#define INT_VECTOR_DOUBLE_FAULT 0x08
#define INT_VECTOR_COPROC_SEG 0x09
#define INT_VECTOR_INVAL_TSS 0x0a
#define INT_VECTOR_SEG_NOT 0x0b
#define INT_VECTOR_STACK_FAULT 0x0c
#define INT_VECTOR_PROTECTION 0x0d
#define INT_VECTOR_PAGE_FAULT 0x0e
#define INT_VECTOR_COPROC_ERR 0x10

/* 系统调用 */
#define INT_VECTOR_SYS_CALL 0x90

#define disable_int_begin()                                  \
  {                                                          \
    int __scoped_interrupt_flag = read_eflags() & EFLAGS_IF; \
    if (__scoped_interrupt_flag) {                           \
      disable_int();                                         \
    }

#define disable_int_end()        \
  if (__scoped_interrupt_flag) { \
    enable_int();                \
  }                              \
  }

void enable_irq(int irq);
void disable_irq(int irq);
void spurious_irq(int irq);
void put_irq_handler(int irq, irq_handler handler);

void exception_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
//: init 8259A interrupt controller
void init_interrupt_controller();

void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

// void division_error();
// void debug_exception();
// void nmi();
// void breakpoint_exception();
// void overflow_exception();
// void bound_range_exceeded();
// void invalid_opcode();
// void device_not_available();
// void double_fault();
// void copr_seg_overrun();
// void invalid_tss();
// void segment_not_present();
// void stack_seg_exception();
// void general_protection();
// void page_fault();
// void floating_point_exception();

/* 中断处理函数 */
void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();

// void syscall_handler();
PUBLIC void sys_call();  // int_handler defined in kernel.asm
PUBLIC void divide_error_handler();

/* 门描述符 */
typedef struct s_gate {
  u16 offset_low;  /* Offset Low */
  u16 selector;    /* Selector */
  u8 dcount;       /* 该字段只在调用门描述符中有效。
                   如果在利用调用门调用子程序时引起特权级的转换和堆栈的改变，需要将外层堆栈中的参数复制到内层堆栈。
                   该双字计数字段就是用于说明这种情况发生时，要复制的双字参数的数量。
                 */
  u8 attr;         /* P(1) DPL(2) DT(1) TYPE(4) */
  u16 offset_high; /* Offset High */
} GATE;

#define GDT_SIZE 128
#define IDT_SIZE 256

// extern	irq_handler	irq_table[NR_IRQ];
// extern	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
// extern	GATE		idt[IDT_SIZE];
