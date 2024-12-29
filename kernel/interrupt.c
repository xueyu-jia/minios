#include <minios/interrupt_x86.h>
#include <minios/proc.h>
#include <minios/assert.h>
#include <minios/x86.h>
#include <minios/page.h>
#include <minios/protect.h>
#include <minios/memman.h>
#include <minios/kstate.h>

irq_handler_t irq_table[NR_IRQ];

static void init_8259A();
static void init_idt();

void init_interrupt_controller() {
    init_8259A();
    //: set default irq handler
    for (int i = 0; i < NR_IRQ; i++) { irq_table[i] = spurious_irq; }

    init_idt();
}

static void init_8259A() {
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
    kprintf("trapped into spurious irq %d\n", irq);
}

void put_irq_handler(int irq, irq_handler_t handler) {
    disable_irq(irq);
    irq_table[irq] = handler;
}

static const char *int_str_table[64] = {"#DE Divide Error",
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

void exception_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    kprintf("[%s] eip=%#08x eflags=%#x cs=%#x err_code=%d from \"%s\" [pid=%d]\n",
            int_str_table[vec_no], eip, eflags, cs, err_code, p_proc_current->task.p_name,
            p_proc_current->task.pid);

    if (kstate_on_init) { unreachable(); }

    do_exit(-1);
    proc_backtrace();

    p_proc_current->task.stat = KILLED;
}

void general_protection_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    kprintf("[%s] eip=%#08x eflags=%#x cs=%#x err_code=%d from \"%s\" [pid=%d]\n",
            int_str_table[vec_no], eip, eflags, cs, err_code, p_proc_current->task.p_name,
            p_proc_current->task.pid);
    typedef struct {
        u32 external : 1;
        u32 table : 2;
        u32 index : 13;
        u32 reserved : 16;
    } selector_error_code_t;
    if (err_code != 0) {
        selector_error_code_t sel_err = *(selector_error_code_t *)&err_code;
        enum {
            DT_GDT,
            DT_LDT,
            DT_IDT,
        };
        const int table_type = (sel_err.table & 0b01)   ? DT_IDT
                               : (sel_err.table & 0b10) ? DT_LDT
                                                        : DT_GDT;
        const char *table_name = ((const char *[3]){"GDT", "LDT", "IDT"})[table_type];
        kprintf("general protection fault is segment related to descriptor table %s, index %d%s\n",
                table_name, sel_err.index,
                sel_err.external ? " , originated externally to the processor" : "");
        if (table_type == DT_GDT || table_type == DT_LDT) {
            const auto desc_table = table_type == DT_GDT ? gdt : p_proc_current->task.context.ldts;
            if (table_type == DT_LDT) {
                int ldt_sel = 0;
                asm volatile("sldt %0" : "=m"(ldt_sel));
                const descriptor_t ldt_desc = gdt[SELECTOR_GET_INDEX(ldt_sel)];
                const descriptor_t *real_ldt = u2ptr(DESC_GET_BASE(ldt_desc));
                if (u2ptr(DESC_GET_BASE(ldt_desc)) != desc_table) {
                    kprintf("unexpected LDT address at 0x%p, expect 0x%p\n", real_ldt, desc_table);
                } else {
                    kprintf("LDT base=0x%p limit=0x%p max-descs=%d\n", DESC_GET_BASE(ldt_desc),
                            DESC_GET_LIMIT(ldt_desc),
                            (DESC_GET_LIMIT(ldt_desc) + 1) / sizeof(descriptor_t));
                }
            }
            const auto desc = desc_table[sel_err.index];
            kprintf("(0x%p) %s[%d] base=0x%p limit=0x%p attr=%#x\n", desc_table, table_name,
                    sel_err.index, DESC_GET_BASE(desc), DESC_GET_LIMIT(desc), DESC_GET_ATTR(desc));
        } else if (table_type == DT_IDT) {
            const auto desc = idt[sel_err.index];
            UNUSED(desc);
            //! TODO: details of idt entry
        } else {
            unreachable();
        }
    }

    if (kstate_on_init) { unreachable(); }

    do_exit(-1);
    proc_backtrace();

    p_proc_current->task.stat = KILLED;
}

void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    kprintf("[%s] eip=%#08x eflags=%#x cs=%#x err_code=%d from \"%s\" [pid=%d]\n",
            int_str_table[vec_no], eip, eflags, cs, err_code, p_proc_current->task.p_name,
            p_proc_current->task.pid);

    typedef struct {
        u32 present : 1;
        u32 write : 1;
        u32 user : 1;
        u32 reserved_write : 1;
        u32 instruction_fetch : 1;
        u32 protection_key : 1;
        u32 shadow_stack : 1;
        u32 reserved1 : 8;
        u32 software_guard_extensions : 1;
        u32 reserved2 : 16;
    } page_fault_error_t;

    page_fault_error_t err = *(page_fault_error_t *)&err_code;
    const uintptr_t pf_va = rcr2();
    kprintf("page fault occurs at virtual address: 0x%p\n", pf_va);
    kprintf("caused by:\n");
    kprintf("    %s\n", err.present ? "page-protection violation" : "non-present page");
    kprintf("    %s\n", err.write ? "write access" : "read access");
    kprintf("    %s, indicates the CPL only\n",
            err.user ? "user-mode access" : "supervisor-mode access");
    if (err.instruction_fetch) {
        kprintf("    instruction fetch, no-execute bit is supported and enabled\n");
    }
    if (err.protection_key) {
        kprintf("    protection-key violation, rights is specified in %s\n",
                err.user ? "PKRU register" : "PKRS MSR");
    }
    if (err.shadow_stack) { kprintf("    shadow stack access\n"); }
    if (err.software_guard_extensions) { kprintf("    SGX violation\n"); }

#ifdef OPT_MMU_COW
    {
        int fault_flag = 0;
        if (!err.present) { fault_flag |= FAULT_NOPAGE; }
        if (!err.write) { fault_flag |= FAULT_WRITE; }
        if (handle_mm_fault(proc_memmap(p_proc_current), pf_va, fault_flag) == 0) {
            refresh_page_cache();
            return;
        }
    }
#endif

    if (kstate_on_init) { unreachable(); }

    do_exit(-1);
    proc_backtrace();

    p_proc_current->task.stat = KILLED;
}

static void init_idt_desc(unsigned char vector, u8 desc_type, int_handler handler,
                          unsigned char privilege);

static void init_idt() {
    idt_ptr.base = ptr2u(idt);
    idt_ptr.limit = sizeof(idt) - 1;

    init_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, divide_error, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, single_step_exception, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception, RPL_USER);
    init_idt_desc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, RPL_USER);
    init_idt_desc(INT_VECTOR_BOUNDS, DA_386IGate, bounds_check, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, inval_opcode, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, copr_not_available, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, inval_tss, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, stack_exception, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_PROTECTION, DA_386IGate, general_protection, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, copr_error, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_SYS_CALL, DA_386IGate, sys_call, RPL_USER);
}

static void init_idt_desc(unsigned char vector, u8 desc_type, int_handler handler,
                          unsigned char privilege) {
    gate_descriptor_t *p_gate = &idt[vector];
    u32 base = (u32)handler;
    p_gate->offset_low = base & 0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;
    p_gate->dcount = 0;
    p_gate->attr = desc_type | (privilege << 5);
    p_gate->offset_high = (base >> 16) & 0xFFFF;
}
