//added by mingxuan 2021-2-28
/*
#include "ksignal.h"

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
*/

#include "../include/ksignal.h"
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"

/*  //deleted by mingxuan 2021-8-20
int do_signal(int sig, void *handler, void* _Handler) {
    if((uint32_t)sig >= NR_SIGNALS) {
        return -1;
    }
    p_proc_current->task.sig_handler[sig] = handler;
    p_proc_current->task._Hanlder = _Handler;
    return 0;
}
*/

//added by mingxuan 2021-8-20
int kern_signal(int sig, void *handler, void* _Handler) {
    if((uint32_t)sig >= NR_SIGNALS) {
        return -1;
    }
    p_proc_current->task.sig_handler[sig] = handler;
    p_proc_current->task._Hanlder = _Handler;
    return 0;
}
//modified by mingxuan 2021-8-20
int do_signal(int sig, void *handler, void* _Handler) {
    kern_signal(sig, handler, _Handler);
}

/*
int do_sigsend(int pid, Sigaction* sigaction_p)
{

    PROCESS* proc = &proc_table[pid];

    if(proc->task.sig_handler[sigaction_p->sig] == NULL ||
        (proc->task.sig_set & (1 << sigaction_p->sig)))
    {
        return -1;
    }
    proc->task.sig_set |= (1 << sigaction_p->sig);
    proc->task.sig_arg[sigaction_p->sig] = sigaction_p->arg;

    return 0;
}
*/
//modified by mingxuan 2021-8-20
int kern_sigsend(int pid, Sigaction* sigaction_p)
{

    PROCESS* proc = &proc_table[pid];

    if(proc->task.sig_handler[sigaction_p->sig] == NULL ||
        (proc->task.sig_set & (1 << sigaction_p->sig)))
    {
        return -1;
    }
    proc->task.sig_set |= (1 << sigaction_p->sig);
    proc->task.sig_arg[sigaction_p->sig] = sigaction_p->arg;

    return 0;
}
//modified by mingxuan 2021-8-20
int do_sigsend(int pid, Sigaction* sigaction_p)
{
    kern_sigsend(pid, sigaction_p);
}

/* //deleted by mingxuan 2021-8-20
void do_sigreturn(int ebp)
{
    STACK_FRAME regs;

    // copy saved regs from stack to  this regs
    // to some operation to compute true address
    // int ebp = msg->data[1];
    int esp_syscall = p_proc_current->task.esp_save_syscall;
    int last_esp = ebp + sizeof(Sigaction) + 8;    //int save esp

    uint16_t user_ss = p_proc_current->task.regs.ss;
    uint16_t kernel_es ;

    // change es to B_ss
    __asm__  (
    "mov %%es, %%ax\n"
    "mov %%ebx, %%es\n"
    : "=a"(kernel_es)
    : "b"(user_ss)
    :
    );

    for(uint32_t* p = &regs, *q = last_esp, i = 0; i < sizeof(regs) / sizeof(uint32_t);i++, p++, q++)
    {
        __asm__ (
        "mov %%eax, %%es:(%%edi)"
        :
        : "a"(*q), "D"(p)
        :
        );
    }

    __asm__ (
            "mov %%eax, %%es\n"
            :
            : "a"(kernel_es)
    );
    memcpy(esp_syscall, &regs, sizeof(STACK_FRAME));
}
*/
//modified by mingxuan 2021-8-20
void kern_sigreturn(int ebp)
{
    STACK_FRAME regs;

    // copy saved regs from stack to  this regs
    // to some operation to compute true address
    // int ebp = msg->data[1];
    int esp_syscall = p_proc_current->task.esp_save_syscall;
    int last_esp = ebp + sizeof(Sigaction) + 8;    //int save esp

    uint16_t user_ss = p_proc_current->task.regs.ss;
    uint16_t kernel_es ;

    // change es to B_ss
    __asm__  (
    "mov %%es, %%ax\n"
    "mov %%ebx, %%es\n"
    : "=a"(kernel_es)
    : "b"(user_ss)
    :
    );

    for(uint32_t* p = &regs, *q = last_esp, i = 0; i < sizeof(regs) / sizeof(uint32_t);i++, p++, q++)
    {
        __asm__ (
        "mov %%eax, %%es:(%%edi)"
        :
        : "a"(*q), "D"(p)
        :
        );
    }

    __asm__ (
            "mov %%eax, %%es\n"
            :
            : "a"(kernel_es)
    );
    memcpy(esp_syscall, &regs, sizeof(STACK_FRAME));
}

//modified by mingxuan 2021-8-20
void do_sigreturn(int ebp)
{
    kern_sigreturn(ebp);
}

int sys_signal() {
    return do_signal(get_arg(1), get_arg(2), get_arg(3));
}

int sys_sigsend(){
    return do_sigsend(get_arg(1), get_arg(2));
}

void sys_sigreturn() {
    do_sigreturn(get_arg(1));
}

void process_signal() {
    /* speed up */
    if(!p_proc_current->task.sig_set) {
        return ;
    }

    int sig = -1, arg;
    for(int i = 0; i < 32; i++) {
        if(p_proc_current->task.sig_set & (1 << i)
            && p_proc_current->task.sig_handler[i] /* for stable */)
        {
            p_proc_current->task.sig_set ^= (1 << i);
            sig = i, arg = p_proc_current->task.sig_arg[i];
            break;
        }
    }
    if(sig == -1) {
        return ;
    }
    //  change this proc(B)'s eip to warper function
    // warp function includes { handler, signal_return }
    // then first execuate handler function
    // then second call signal_return to kernel

    STACK_FRAME regs;
    PROCESS* proc = p_proc_current;
    memcpy(&regs, &proc->task.regs, sizeof(STACK_FRAME));
    Sigaction sigaction = {
        .sig = sig,
        .handler = proc->task.sig_handler[sig],
        .arg = proc->task.sig_arg[sig]
    };

    disable_int();

    uint16_t B_ss = regs.ss & 0xfffc;
    uint16_t A_es ;

    /* change es to B_ss */
    __asm__  (
        "mov %%es, %%ax\n"
        "mov %%ebx, %%es\n"
        : "=a"(A_es)
        : "b"(B_ss)
        :
    );

    /* save context */
    int start = *(uint32_t*)(proc->task.esp_save_syscall + 16*4) - sizeof(regs);
    for(uint32_t* p = start, *sf = proc->task.esp_save_syscall, i=0; i<sizeof(regs) / sizeof(uint32_t) ; i++,p++, sf++) {
        __asm__ (
            "mov %%eax, %%es:(%%edi)"
            :
            : "a"(*sf), "D"(p)
            :
        );
    }

    /* push para */
    start -= sizeof(Sigaction);
    for(uint32_t* p = start, *sf = &sigaction, i = 0; i < sizeof(Sigaction) / sizeof(uint32_t) ;i++, p++, sf++) {
        __asm__ (
            "mov %%eax, %%es:(%%edi)"
            :
            : "a"(*sf), "D"(p)
            :
        );
    }

    /* Handler return address */
    start -= sizeof(uint32_t);

    /* switch to Handler */
    uint32_t *context_p = proc->task.esp_save_syscall;
    *(context_p + 13) =  proc->task._Hanlder; /* eip */
    *(context_p + 16) = start;                /* esp */

    /*  reverse  */
    __asm__ __volatile__ (
        "mov %0, %%es\n"
        :
        : "r"(A_es)
        :
    );
    enable_int();
}
