#include <kernel/interrupt_x86.h>
#include <kernel/proc.h>
#include <kernel/x86.h>
// #include <kernel/stdio.h>

irq_handler irq_table[NR_IRQ];
u8 idt_ptr[6]; // 0~15:Limit  16~47:Base
GATE idt[IDT_SIZE];

PRIVATE void init_8259A();
PRIVATE void init_idt();

static const char *err_description[64] = {
    "#DE Divide Error",
    "#DB RESERVED",
    "—  NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode (Undefined Opcode)",
    "#NM Device Not Available (No Math Coprocessor)",
    "#DF Double Fault",
    "    Coprocessor Segment Overrun (reserved)",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection",
    "#PF Page Fault",
    "—  (Intel reserved. Do not use.)",
    "#MF x87 FPU Floating-Point Error (Math Fault)",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD Floating-Point Exception"};

PUBLIC void init_interrupt_controller() {
    init_8259A();
    //: set default irq handler
    for (int i = 0; i < NR_IRQ; i++) { irq_table[i] = spurious_irq; }

    init_idt();
}

// void init_protect_mode(){
//     init_interrupt_controller();
//     init_idt();
// }

PRIVATE void init_8259A() {
    //: NOTE: reference can be found at:
    //: https://github.com/intel/CODK-A-X86/blob/master/external/zephyr/drivers/interrupt_controller/i8259.c

    //: ICW1: init
    //: NOTE: require ICW4 & initiates initialization sequence
    outb(INT_M_CTL, 0x11);
    outb(INT_S_CTL, 0x11);

    //: ICW2: set int vector offset
    //: NOTE: master begins at irq 0, slave begins at irq 8
    outb(INT_M_CTLMASK, INT_VECTOR_IRQ0);
    outb(INT_S_CTLMASK, INT_VECTOR_IRQ8);

    //: ICW3: set cascade mode
    //: NOTE: cascade at irq 4, slave connect to irq 2
    outb(INT_M_CTLMASK, 0x4);
    outb(INT_S_CTLMASK, 0x2);

    //: ICW4: set ctrl word
    //: NOTE: set mode 8086
    outb(INT_M_CTLMASK, 0x1);
    outb(INT_S_CTLMASK, 0x1);

    //: OCW1: ints barrier
    //: NOTE: barrier all interrupts
    outb(INT_M_CTLMASK, 0xff);
    outb(INT_S_CTLMASK, 0xff);
}

void enable_irq(int irq) {
    u8 mask = 1 << (irq % 8);
    if (irq < 8) {
        outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) & ~mask);
    } else {
        outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) & ~mask);
    }
}

void disable_irq(int irq) {
    u8 mask = 1 << (irq % 8);
    if (irq < 8)
        outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) | mask);
    else
        outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) | mask);
}

void spurious_irq(int irq) {
    // klog("spurious_irq: %d", irq);
    disp_str("spurious_irq: ");
    disp_int(irq);
    disp_str("\n");
}

void put_irq_handler(int irq, irq_handler handler) {
    disable_irq(irq);
    irq_table[irq] = handler;
}

// void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
// {
//     klog(
//         "[Exception %s] eip=%08x eflags=0x%x cs=0x%x err_code=%d from "
//         "pid=%d",
//         err_description[vec_no],
//         eip,
//         eflags,
//         cs,
//         err_code,
//         p_proc_current->task.pid);

// p_proc_current->task.stat = KILLED;
// }

PUBLIC void exception_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs,
                              u32 eflags) {
    int i;
    int text_color = 0x74; /* 灰底红字 */

    /* 通过打印空格的方式清空屏幕的前五行，并把 disp_pos 清零 */
    disp_pos = 0;
    for (i = 0; i < 80 * 5; i++) { disp_str(" "); }
    disp_pos = 0;

    disp_color_str("Exception! --> ", text_color);
    disp_color_str(err_description[vec_no], text_color);
    disp_color_str("\n\n", text_color);
    disp_color_str("EFLAGS:", text_color);
    disp_int(eflags);
    disp_color_str("CS:", text_color);
    disp_int(cs);
    disp_color_str("EIP:", text_color);
    disp_int(eip);
    disp_str("pid=");
    disp_int(proc2pid(p_proc_current));
    if (err_code != 0xFFFFFFFF) {
        disp_color_str("Error code:", text_color);
        disp_int(err_code);
    }
    if (vec_no == 6) { // undefined op
    }
    disp_str("\n");
    // halt();
    do_exit(-1);
    proc_backtrace();
    p_proc_current->task.stat = KILLED;
}

/*======================================================================*
                                                        divide error handler
 *======================================================================*/
// used for testing if a exception handler can be interrupted rightly, so it's
// not a real divide_error handler now. added by xw, 18/12/22
PUBLIC void divide_error_handler() {
    int vec_no, err_code, eip, cs, eflags;
    int i, j;

    asm volatile(
        "mov 8(%%ebp), %0\n\t"  // get vec_no from stack
        "mov 12(%%ebp), %1\n\t" // get err_code from stack
        "mov 16(%%ebp), %2\n\t" // get eip from stack
        "mov 20(%%ebp), %3\n\t" // get cs from stack
        "mov 24(%%ebp), %4\n\t" // get eflags from stack
        : "=r"(vec_no), "=r"(err_code), "=r"(eip), "=r"(cs), "=r"(eflags));
    exception_handler(vec_no, err_code, eip, cs, eflags);

    while (1) {
        disp_str("Loop in divide error handler...\n");
        i = 100;
        while (--i) {
            j = 1000;
            while (--j) {}
        }
    }
}

PRIVATE void init_idt_desc(unsigned char vector, u8 desc_type,
                           int_handler handler, unsigned char privilege);

PRIVATE void init_idt() {
    // idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt
    // 的参数。
    u16 *p_idt_limit = (u16 *)(&idt_ptr[0]);
    u32 *p_idt_base = (u32 *)(&idt_ptr[2]);
    *p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *p_idt_base = (u32)&idt;

    // 全部初始化成中断门(没有陷阱门)
    init_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, divide_error, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, single_step_exception,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception,
                  PRIVILEGE_USER);

    init_idt_desc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, PRIVILEGE_USER);

    init_idt_desc(INT_VECTOR_BOUNDS, DA_386IGate, bounds_check, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, inval_opcode,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, copr_not_available,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, inval_tss, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, stack_exception,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_PROTECTION, DA_386IGate, general_protection,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, copr_error,
                  PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_SYS_CALL, DA_386IGate, sys_call, PRIVILEGE_USER);
}

/*======================================================================*
                             init_idt_desc
 *----------------------------------------------------------------------*
 初始化 386 中断门
 *======================================================================*/
PRIVATE void init_idt_desc(unsigned char vector, u8 desc_type,
                           int_handler handler, unsigned char privilege) {
    GATE *p_gate = &idt[vector];
    u32 base = (u32)handler;
    p_gate->offset_low = base & 0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;
    p_gate->dcount = 0;
    p_gate->attr = desc_type | (privilege << 5);
    p_gate->offset_high = (base >> 16) & 0xFFFF;
}
