#include <minios/const.h>
#include <minios/x86.h>
#include <minios/regs.h>
#include <minios/assert.h>

void init_context_frame(CONTEXT_FRAME *context_frame, u32 eip, u32 eflags) {
    context_frame->eip = eip;
    context_frame->eflags = eflags;
}

static void assign_ldt(int pid, cpu_context *context, u8 rpl) {
    context->ldt_sel = MAKE_SELECTOR(INDEX_LDT_FIRST + pid);
    context->ldts[INDEX_LDT_C] = gdt[SELECTOR_GET_INDEX(SELECTOR_KERNEL_CS)];
    context->ldts[INDEX_LDT_C].attr0 = DA_C | (rpl << 5);
    context->ldts[INDEX_LDT_RW] = gdt[SELECTOR_GET_INDEX(SELECTOR_KERNEL_DS)];
    context->ldts[INDEX_LDT_RW].attr0 = DA_DRW | (rpl << 5);
}

void init_cpu_context(cpu_context *context, int pid, u32 int_eip, u32 int_esp, u32 context_eip,
                      u8 rpl) {
    assert(is_rpl_supported(rpl));

    assign_ldt(pid, context, rpl);

    auto frame = context->esp_save_int;

    frame->cs = SELECTOR_DEFAULT_CS(rpl);
    frame->ds = SELECTOR_DEFAULT_DS(rpl);
    frame->es = SELECTOR_DEFAULT_ES(rpl);
    frame->fs = SELECTOR_DEFAULT_FS(rpl);
    frame->ss = SELECTOR_DEFAULT_SS(rpl);
    frame->gs = SELECTOR_DEFAULT_GS(rpl);
    frame->eflags = EFLAGS(IF, IOPL(rpl == RPL_USER ? 0 : 1));
    frame->esp = int_esp;
    frame->eip = int_eip;

    init_context_frame(context->esp_save_context, context_eip, EFLAGS(IF, IOPL(1)));
}

int get_ring_level() {
    int ring_level = 0;
    asm volatile("mov %%cs, %0\nand $0x3, %0" : "=a"(ring_level));
    return ring_level;
}
