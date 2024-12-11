/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-16 14:30:31
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 18:32:29
 * @FilePath: /minios/os/kernel/proc_init.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/hd.h>
#include <kernel/mmap.h>
#include <kernel/pagetable.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <kernel/x86.h>
#include <klib/string.h>

PRIVATE void set_rpl(cpu_context* context, int pid, u32 rpl);
PRIVATE void init_proc_pages(PROCESS* p_proc);
PRIVATE void init_proc_page_addr(u32 low, u32 high, PROCESS* proc);
PRIVATE void init_a_PCB(PROCESS* pcb);

PUBLIC PROCESS* alloc_PCB() {
    PROCESS* p = proc_table + NR_K_PCBS; // 跳过前NR_K_PCBS个
    for (int i = NR_K_PCBS; i < NR_PCBS; i++) {
        if (p->task.stat == FREE) {
            // disable_int();
            init_a_PCB(p);
            p->task.stat = SLEEPING;
            // p->task.pid = i;
            // enable_int();
            return p;
        }
        p++;
    }
    return NULL;
}

PUBLIC PROCESS* alloc_task_PCB() {
    PROCESS* p;
    // p = proc_table+NR_K_PCBS;//跳过前NR_K_PCBS个
    p = proc_table;
    for (int i = 0; i < NR_K_PCBS + 1; i++) {
        if (p->task.stat == FREE) {
            init_a_PCB(p);
            p->task.stat = SLEEPING;
            return p;
        }
        p++;
    }
    return NULL;
}

PUBLIC PROCESS* alloc_init_PCB() {
    PROCESS* p;
    // p = proc_table+NR_K_PCBS;//跳过前NR_K_PCBS个
    p = &proc_table[PID_INIT];
    if (p->task.stat == FREE) {
        init_a_PCB(p);
        p->task.stat = SLEEPING;
        return p;
    }

    disp_str("\nalloc init PCB failed\n");
    while (1) {};
    return NULL;
}

PUBLIC void free_PCB(PROCESS* p) {
    p->task.stat = FREE;
}

/**
 * @description: 初始化进程基本信息
 * @param {PROCESS} *proc
 * @param {char} name
 * @param {enum proc_stat} stat
 * @param {int} pid
 * @param {int} is_rt
 * @param {int} priority_or_nice
 * @return {*}
 * @note: name, pid, proc stat, realtime or not, priority or nice
 */
PUBLIC void init_process(PROCESS* proc, char name[32], enum proc_stat stat,
                         int pid, int is_rt, int priority_or_nice) {
    strcpy(proc->task.p_name, name); // 名称
    proc->task.pid = pid;            // pid
    proc->task.stat = stat;          // 初始化状态 -1表示未初始化
    proc->task.ticks = proc->task.priority = 1; // 时间片和优先级

    // CFS info
    proc->task.is_rt = is_rt;
    if (is_rt) {
        proc->task.rt_priority = priority_or_nice;
    } else {
        proc->task.nice = priority_or_nice;
        proc->task.weight = nice_to_weight[proc->task.nice + 20];
    }
    proc->task.vruntime = 0;
    proc->task.sum_cpu_use = 0;
    proc->task.cpu_use = 0;
}

/**
 * @description: 初始化进程的上下文
 * @param {cpu_context} *context
 * @param {int} pid should be correct
 * @return {*}
 * @note
            cpu context switch to the "esp_save_context" whe scheduler switch
 this proc; cpu context switch to the "esp_save_int" whe cpu's rpl change to
 task level or kernel level (interrupt)
 */
PUBLIC void init_user_cpu_context(cpu_context* context, int pid) {
    PROCESS* pcb = pid2proc(pid);
    pcb->task.context.esp_save_int = proc_stack_frame(pcb);
    pcb->task.context.esp_save_context = proc_context_frame(pcb);
    set_rpl(context, pid, RPL_USER);
}

/**
 * @description: set the context's rpl
 * @param {cpu_context} *context
 * @param {int} pid should be correct
 * @param {u32} rpl: could be RPL_USER, RPL_TASK or RPL_KERN
 * @return {*}
 * @note context's ldt, esp_save_int and esp_save_context will be set
            but the eip and esp in the esp_save_int will be remaind;

 */
PRIVATE void set_rpl(cpu_context* context, int pid, u32 rpl) {
    init_cpu_context(context, pid, context->esp_save_int->eip,
                     context->esp_save_int->esp, (u32)restart_restore, rpl);
}

PUBLIC int kthread_create(char* name, void* func, int is_rt, int priority,
                          int rtpriority_or_nice) {
    // 禁用中断
    PROCESS* p_proc;
    if (strcmp(name, "initial") == 0)
        p_proc = alloc_init_PCB();
    else
        p_proc = alloc_task_PCB();

    if (p_proc == NULL) {
        disp_color_str("kthread_create: alloc PCB error\n", 0x74);
        return -1;
    }

    init_proc_pages(p_proc);

    init_cpu_context(&p_proc->task.context, p_proc->task.pid, (u32)func,
                     StackLinBase, (u32)restart_restore, priority);

    // pid不应该和PCB的位置绑定，需要一个单独的alloc_pid() mark lirong
    init_process(p_proc, name, READY, p_proc->task.pid, is_rt,
                 rtpriority_or_nice);

    // 进程树初始化
    p_proc->task.tree_info.type = TYPE_PROCESS; // 当前是进程还是线程
    p_proc->task.tree_info.real_ppid = -1; // 亲父进程，创建它的那个进程
    p_proc->task.tree_info.ppid = -1;      // 当前父进程
    p_proc->task.tree_info.child_p_num = 0; // 子进程数量
    p_proc->task.tree_info.child_t_num = 0; // 子线程数量
    p_proc->task.tree_info.text_hold = 1;   // 是否拥有代码
    p_proc->task.tree_info.data_hold = 1;   // 是否拥有数据

    // 恢复中断
    //  intr_restore(intr);
    return p_proc->task.pid;
}

// 调用此函数分配页表
PRIVATE void init_proc_pages(PROCESS* p_proc) {
    int pid = p_proc->task.pid;
    if (0 != init_proc_page(pid)) {
        disp_color_str("kernel_main Error:init_proc_page", 0x74);
        return;
    }
    p_proc->task.memmap.heap_lin_base = HeapLinBase;
    p_proc->task.memmap.heap_lin_limit = HeapLinBase; // 堆的界限将会一直动态变化

    p_proc->task.memmap.stack_child_limit =
        StackLinLimitMAX; // add by visual 2016.5.27
    p_proc->task.memmap.stack_lin_base = StackLinBase;
    p_proc->task.memmap.stack_lin_limit =
        StackLinBase -
        0x4000; // 栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
    p_proc->task.memmap.arg_lin_base = ArgLinBase; // 参数内存基址
    p_proc->task.memmap.arg_lin_limit = ArgLinLimitMAX;
    p_proc->task.memmap.kernel_lin_base = KernelLinBase;
    p_proc->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size;
    list_init(&p_proc->task.memmap.vma_map);
    init_mem_page(&p_proc->task.memmap.anon_pages, MEMPAGE_AUTO);
    init_proc_page_addr(p_proc->task.memmap.stack_lin_limit,
                        p_proc->task.memmap.stack_lin_base, p_proc);
    init_proc_page_addr(p_proc->task.memmap.arg_lin_base,
                        p_proc->task.memmap.arg_lin_limit, p_proc);
}

// [start,end) 地址段分配页表，如果不存在分配物理页并填写页表，否则忽略
// 固定地址start<end
PRIVATE void init_proc_page_addr(u32 low, u32 high, PROCESS* proc) {
    kern_mmap(proc, NULL, low, high - low, PROT_READ | PROT_WRITE, MAP_PRIVATE,
              0);
}

PRIVATE void init_a_PCB(PROCESS* pcb) {
    memset(pcb, 0, sizeof(PROCESS));
    pcb->task.stat = SLEEPING;
    pcb->task.pid = pcb - proc_table;

    // 开辟上下文帧的空间
    char* frame = (char*)(pcb + 1);
    frame -= sizeof(STACK_FRAME);
    pcb->task.context.esp_save_int = (STACK_FRAME*)frame;
    // 开辟中断帧的空间
    frame -= sizeof(CONTEXT_FRAME);
    pcb->task.context.esp_save_context = (CONTEXT_FRAME*)frame;
    init_user_cpu_context(&pcb->task.context, pcb->task.pid);
    for (int j = 0; j < NR_FILES; j++) { pcb->task.filp[j] = 0; }
}
