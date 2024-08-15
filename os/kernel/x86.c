#include <kernel/const.h>
#include <kernel/x86.h>
#include <kernel/string.h>

// 初始化中断帧
void init_int_frame(
	STACK_FRAME *frame,
	u32 cs,
	u32 ds,
	u32 es,
	u32 fs,
	u32 ss,
	u32 gs,
	u32 eflags,
	u32 esp,
	u32 eip	)
{
	frame->cs = cs;
	frame->ds = ds;
	frame->es = es;
	frame->fs = fs;
	frame->ss = ss;
	frame->gs = gs;
	frame->eflags =eflags;    /* IF=1, IOPL=1 */
	frame->esp = esp;
	frame->eip = eip;
}

// 初始化上下文帧, 移入arch
void init_context_frame(CONTEXT_FRAME* context_frame, u32 eip, u32 eflags)
{
	context_frame->eip = eip;
	context_frame->eflags = eflags;
}

void alloc_ldt(int pcb_nr, cpu_context *context, int rpl)
{
	u8 privilege = (u8)rpl;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	while(pcb_nr > 0){
			selector_ldt += 1 << 3;
			pcb_nr--;
	}
    context->ldt_sel = selector_ldt;
    memcpy(&context->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
    context->ldts[0].attr1 = DA_C | privilege << 5;
    memcpy(&context->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
    context->ldts[1].attr1 = DA_DRW | privilege << 5;

}
void init_cpu_context(cpu_context *context, int pid, u32 int_eip, u32 int_esp, u32 context_eip, int rpl)
{
    if(rpl != RPL_USER && rpl != RPL_TASK &&rpl != RPL_KRNL){
        // ! panic
        disp_str("\ninit cpu context error");
        while(1){};
    }
	alloc_ldt(pid, context, rpl);

    u32 eflags = (rpl != RPL_USER)? 0x1202 : 0x202;
    init_int_frame(
        context->esp_save_int,
        common_cs|rpl, common_ds|rpl, common_es|rpl,
        common_fs|rpl, common_ss|rpl, common_gs|rpl,
        eflags, (u32)int_esp,
        (u32)int_eip
		);

	init_context_frame(context->esp_save_context, context_eip, 0x1202);

}
