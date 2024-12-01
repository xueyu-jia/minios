/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-15 09:40:51
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 23:09:39
 * @FilePath: /minios/os/include/kernel/x86.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#pragma once

#include <kernel/protect.h>
#include <kernel/stdint.h>
#include <kernel/x86-asm.h>

// ring0权限
#define kern_cs ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_ds ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_es ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_fs ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_ss ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_KRNL
#define kern_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_KRNL
// ring1权限
#define task_cs ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_ds ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_es ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_fs ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_ss ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK
#define task_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK

// 切换进程要保存的寄存器
typedef struct s_stackframe { /* proc_ptr points here
                                 ↑ Low			*/
  u32 gs;                     /* ┓						│                     */
  u32 fs;                     /* ┃						│                     */
  u32 es;                     /* ┃						│                     */
  u32 ds;                     /* ┃						│                     */
  u32 edi;                    /* ┃						│                    */
  u32 esi;                    /* ┣ pushed by save()				│
                               */
  u32 ebp;                    /* ┃						│                    */
  u32 kernel_esp;             /* <- 'popad' will ignore it			│
                               */
  u32 ebx;     /* ┃                    ↑栈从高地址往低地址增长*/
  u32 edx;     /* ┃						│                    */
  u32 ecx;     /* ┃						│                    */
  u32 eax;     /* ┛						│                    */
  u32 retaddr; /* return address for assembly code save()	│
                */
  u32 error;   // add by lcy 2023.10.26
  u32 eip;     /*  ┓						│
                */
  u32 cs;      /*  ┃						│                     */
  u32 eflags;  /*  ┣ these are pushed by CPU during interrupt	│
                */
  u32 esp;     /*  ┃						│
                */
  u32 ss;      /*  ┛						┷High     */
} __attribute__((packed)) STACK_FRAME;
// } STACK_FRAME;

typedef struct CONTEXT_FRAME {
  u32 edi;
  u32 esi;
  u32 ebp;
  u32 esp;
  u32 ebx;
  u32 edx;
  u32 ecx;
  u32 eax;     // 以上均由pushad压栈
  u32 eflags;  // 由 pushfd 压栈
  u32 eip;     // 由函数调用自动压栈
} __attribute__((packed)) CONTEXT_FRAME;
// } CONTEXT_FRAME;
/*
因为x86版本使用偏移量访问变量，所以该结构体内部成员请保持原有顺序
注意：sconst.inc文件中规定了变量间的偏移值，新添变量不要破坏原有顺序结构
*/
typedef struct cpu_context {
  u16 ldt_sel;               /* gdt selector giving ldt base and limit */
  DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

  STACK_FRAME* esp_save_int;  // 需要保存的寄存器，定义在各个架构的processor.h中
  // char* esp_save_int;		//to save the position of esp in the
  // kernel stack of the process added by xw, 17/12/11 char* esp_save_syscall;
  // //to save the position of esp in the kernel stack of the process
  CONTEXT_FRAME* esp_save_context;  // to save the position of esp in the kernel
                                    // stack of the process

} cpu_context;  // 加上关键字__attribute__((packed))反而报错了

PUBLIC void init_cpu_context(cpu_context* context, int pid, u32 int_eip,
                             u32 int_esp, u32 context_eip, u32 rpl);
PUBLIC u32 get_ring_level();
