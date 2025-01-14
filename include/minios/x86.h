#pragma once

#include <minios/protect.h>
#include <klib/stdint.h>
#include <compiler.h>

typedef struct {
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

typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;    // 以上均由pushad压栈
    uint32_t eflags; // 由 pushfd 压栈
    uint32_t eip;    // 由函数调用自动压栈
} PACKED context_frame_t;

typedef struct {
    u16 ldt_sel;                 /* gdt selector giving ldt base and limit */
    descriptor_t ldts[LDT_SIZE]; /* local descriptors for code and data */

    stack_frame_t* esp_save_int; // 需要保存的寄存器，定义在各个架构的processor.h中
    // char* esp_save_int;		//to save the position of esp in the
    // kernel stack of the process added by xw, 17/12/11 char* esp_save_syscall;
    // //to save the position of esp in the kernel stack of the process
    context_frame_t* esp_save_context; // to save the position of esp in the
                                       // kernel stack of the process

} cpu_context_t; // 加上关键字 __attribute__((packed)) 反而报错了

void init_cpu_context(cpu_context_t* context, int pid, u32 int_eip, u32 int_esp, u32 context_eip,
                      u8 rpl);
int get_ring_level();
