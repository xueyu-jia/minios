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

#include <minios/protect.h>
#include <minios/x86-asm.h>
#include <klib/stdint.h>

typedef struct stack_frame {
    uint32_t gs;         //<! ━┓
    uint32_t fs;         //<!  ┃
    uint32_t es;         //<!  ┃
    uint32_t ds;         //<!  ┃
    uint32_t edi;        //<!  ┃
    uint32_t esi;        //<!  ┣━┫ push by `save`
    uint32_t ebp;        //<!  ┃
    uint32_t kernel_esp; //<!  ┣ ignored by popad
    uint32_t ebx;        //<!  ┃
    uint32_t edx;        //<!  ┃
    uint32_t ecx;        //<!  ┃
    uint32_t eax;        //<! ━┫
    uint32_t retaddr;    //<!  ┣ retaddr for `save`
    uint32_t error;      //<!  ┃
    uint32_t eip;        //<! ━┫
    uint32_t cs;         //<!  ┃
    uint32_t eflags;     //<!  ┣━┫ push by interrupt
    uint32_t esp;        //<!  ┃
    uint32_t ss;         //<! ━┛
} PACKED stack_frame_t;

typedef struct CONTEXT_FRAME {
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;    // 以上均由pushad压栈
    u32 eflags; // 由 pushfd 压栈
    u32 eip;    // 由函数调用自动压栈
} __attribute__((packed)) CONTEXT_FRAME;
// } CONTEXT_FRAME;
/*
因为x86版本使用偏移量访问变量，所以该结构体内部成员请保持原有顺序
注意：sconst.inc文件中规定了变量间的偏移值，新添变量不要破坏原有顺序结构
*/
typedef struct cpu_context {
    u16 ldt_sel;                 /* gdt selector giving ldt base and limit */
    descriptor_t ldts[LDT_SIZE]; /* local descriptors for code and data */

    stack_frame_t* esp_save_int; // 需要保存的寄存器，定义在各个架构的processor.h中
    // char* esp_save_int;		//to save the position of esp in the
    // kernel stack of the process added by xw, 17/12/11 char* esp_save_syscall;
    // //to save the position of esp in the kernel stack of the process
    CONTEXT_FRAME* esp_save_context; // to save the position of esp in the
                                     // kernel stack of the process

} cpu_context; // 加上关键字__attribute__((packed))反而报错了

void init_cpu_context(cpu_context* context, int pid, u32 int_eip, u32 int_esp, u32 context_eip,
                      u8 rpl);
int get_ring_level();
