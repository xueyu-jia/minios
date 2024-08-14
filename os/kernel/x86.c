/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 15:38:21
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-14 15:50:15
 * @FilePath: /minios/os/kernel/x86.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include <kernel/const.h>
#include <kernel/x86.h>
#include <kernel/type.h>
#include <kernel/string.h>

PRIVATE void init_context_frame(CONTEXT_FRAME* context_frame, u32 eip, u32 eflags);
PRIVATE void alloc_ldt(int pcb_nr, cpu_context *context, int priority);
PUBLIC void init_cpu_context(
    cpu_context *context,
    int pid,
    void *func,
    u32 stackLinBase,
    u32 restart_restore,
    int priority
);
PRIVATE void init_int_frame(
	STACK_FRAME *frame,
	u32 cs,
	u32 ds,
	u32 es,
	u32 fs,
	u32 ss,
	u32 gs,
	u32 eflags,
	u32 esp,
	u32 eip
);


/**
 * @description: 初始化cpu的寄存器
 * @return {*}
 * @note:
 */
PUBLIC void init_cpu_context(
    cpu_context *context,
    int pid,
    void *func,
    u32 stackLinBase,
    u32 restart_restore,
    int priority
)
{
	alloc_ldt(pid, context, priority);

	if(priority == PRIVILEGE_KRNL){
			init_int_frame(context->esp_save_int,
			kern_cs, kern_ds, kern_es,
			kern_fs, kern_ss, kern_gs,
			0x1202, (u32)stackLinBase,
			(u32)func
		);
	}else if(priority == PRIVILEGE_TASK){
		init_int_frame(context->esp_save_int,
			task_cs, task_ds, task_es,
			task_fs, task_ss, task_gs,
			0x1202, (u32)stackLinBase,
			(u32)func
		);
	}else if(priority == PRIVILEGE_USER){
		init_int_frame(context->esp_save_int,
			0, 0, 0,
			0, 0, 0,
			0x0202, (u32)0,
			(u32)func
		);
	}else{
		while(1){
			// mark todo 错误处理
		}
	}

	init_context_frame(context->esp_save_context, restart_restore, 0x1202);

}

PRIVATE void alloc_ldt(int pcb_nr, cpu_context *context, int priority)
{
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	while(pcb_nr > 0){
			selector_ldt += 1 << 3;
			pcb_nr--;
	}
	// disp_int(selector_ldt); // mark debug
	if(priority == PRIVILEGE_USER){
		// user privilege
		context->ldt_sel = selector_ldt;
		memcpy(&context->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
		context->ldts[0].attr1 = DA_C | PRIVILEGE_USER << 5;
		memcpy(&context->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
		context->ldts[1].attr1 = DA_DRW | PRIVILEGE_USER << 5;
	}else if(priority == PRIVILEGE_TASK){
		// task privilege
		context->ldt_sel = selector_ldt;
		memcpy(&context->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
		context->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&context->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
		context->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
	}else if(priority == PRIVILEGE_KRNL){
		// kernel privilege
		context->ldt_sel = selector_ldt;
		memcpy(&context->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
		context->ldts[0].attr1 = DA_C | PRIVILEGE_KRNL << 5;
		memcpy(&context->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
		context->ldts[1].attr1 = DA_DRW | PRIVILEGE_KRNL << 5;
	}

}


/**
 * @description: 初始化CPU上下文帧
 * @param {CONTEXT_FRAME*} context_frame
 * @param {u32} eip
 * @param {u32} eflags
 * @return {*}
 * @note:
 */
PRIVATE void init_context_frame(CONTEXT_FRAME* context_frame, u32 eip, u32 eflags)
{
	context_frame->eip = eip;
	context_frame->eflags = eflags;
}

/**
 * @description: 初始化中断帧
 * @return {*}
 * @note:
 */
PRIVATE void init_int_frame(
	STACK_FRAME *frame,
	u32 cs,
	u32 ds,
	u32 es,
	u32 fs,
	u32 ss,
	u32 gs,
	u32 eflags,
	u32 esp,
	u32 eip
)
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
