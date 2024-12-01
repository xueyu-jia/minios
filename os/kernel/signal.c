// added by mingxuan 2021-2-28

#include <kernel/ksignal.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <klib/string.h>

/*  //deleted by mingxuan 2021-8-20
int do_signal(int sig, void *handler, void* _Handler) {
    if((u32)sig >= NR_SIGNALS) {
        return -1;
    }
    p_proc_current->task.sig_handler[sig] = handler;
    p_proc_current->task._Hanlder = _Handler;
    return 0;
}
*/

// added by mingxuan 2021-8-20
int kern_signal(int sig, void* handler, void* _Handler) {
  if ((u32)sig >= NR_SIGNALS) {
    return -1;
  }
  p_proc_current->task.sig_handler[sig] = handler;
  p_proc_current->task._Hanlder = _Handler;
  return 0;
}
// modified by mingxuan 2021-8-20
int do_signal(int sig, void* handler, void* _Handler) {
  return kern_signal(sig, handler, _Handler);
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
// modified by mingxuan 2021-8-20
int kern_sigsend(int pid, Sigaction* sigaction_p) {
  PROCESS* proc = &proc_table[pid];

  if (proc->task.sig_handler[sigaction_p->sig] == NULL ||
      (proc->task.sig_set & (1 << sigaction_p->sig))) {
    return -1;
  }
  proc->task.sig_set |= (1 << sigaction_p->sig);
  proc->task.sig_arg[sigaction_p->sig] = sigaction_p->arg;

  return 0;
}
// modified by mingxuan 2021-8-20
int do_sigsend(int pid, Sigaction* sigaction_p) {
  return kern_sigsend(pid, sigaction_p);
}

// modified by mingxuan 2021-8-20
void kern_sigreturn(int ebp) {
  STACK_FRAME regs;

  // copy saved regs from stack to  this regs
  // to some operation to compute true address
  // int ebp = msg->data[1];
  STACK_FRAME* esp_syscall = p_proc_current->task.context.esp_save_int;
  u32 last_esp = (ebp + sizeof(Sigaction) + 8);  // int save esp

  // u16 user_ss = p_proc_current->task.regs.ss;
  u16 user_ss = p_proc_current->task.context.esp_save_int->ss;
  u16 kernel_es;

  // change es to B_ss
  __asm__(
      "mov %%es, %%ax\n"
      "mov %%ebx, %%es\n"
      : "=a"(kernel_es)
      : "b"(user_ss)
      :);

  //! FIXME: signal02和signal02能通过，signal01仍然不能通过
  regs = *(STACK_FRAME*)last_esp;
  // for(u32 *p = (u32*)&regs, *q = (u32*)last_esp, i = 0; i <
  // sizeof(regs)/sizeof(u32);i++, p++, q++)
  // {
  //     __asm__ (
  //     "mov %%eax, %%es:(%%edi)"
  //     :
  //     : "a"(*q), "D"(p)
  //     :
  //     );
  // }

  __asm__("mov %%eax, %%es\n" : : "a"(kernel_es));
  memcpy(esp_syscall, &regs, sizeof(STACK_FRAME));
}

// modified by mingxuan 2021-8-20
void do_sigreturn(int ebp) { kern_sigreturn(ebp); }

int sys_signal() {
  return do_signal(get_arg(1), (void*)get_arg(2), (void*)get_arg(3));
}

int sys_sigsend() { return do_sigsend(get_arg(1), (Sigaction*)get_arg(2)); }

void sys_sigreturn() { do_sigreturn(get_arg(1)); }

void process_signal() {
  /* speed up */
  if (!p_proc_current->task.sig_set) {
    return;
  }

  int sig = -1, arg;
  for (int i = 0; i < 32; i++) {
    if (p_proc_current->task.sig_set & (1 << i) &&
        p_proc_current->task.sig_handler[i] /* for stable */) {
      p_proc_current->task.sig_set ^= (1 << i);
      sig = i, arg = p_proc_current->task.sig_arg[i];
      break;
    }
  }
  if (sig == -1) {
    return;
  }
  // add default signal
  void* handler = p_proc_current->task.sig_handler[sig];
  if (handler == SIG_DFL) {
    switch (sig) {
      case SIGINT:
        do_exit(sig);
        break;
      default:
        break;
    }
  }

  //  change this proc(B)'s eip to warper function
  // warp function includes { handler, signal_return }
  // then first execuate handler function
  // then second call signal_return to kernel

  STACK_FRAME regs;
  STACK_FRAME* regs1;
  PROCESS* proc = p_proc_current;
  // memcpy(&regs, &proc->task.regs, sizeof(STACK_FRAME));
  regs1 = p_proc_current->task.context.esp_save_int;
  Sigaction sigaction = {.sig = sig,
                         .handler = proc->task.sig_handler[sig],
                         .arg = proc->task.sig_arg[sig]};

  disable_int();

  // u16 B_ss = regs.ss & 0xfffc;
  u16 B_ss = regs1->ss & 0xfffc;
  u16 A_es;

  /* change es to B_ss */
  __asm__(
      "mov %%es, %%ax\n"
      "mov %%ebx, %%es\n"
      : "=a"(A_es)
      : "b"(B_ss)
      :);

  //! 在消除WARNING时，以下代码逻辑比较混乱，重构前测试signal01无法通过，重构后也无法通过
  //! signal部分代码可能有较大问题
  /* save context */
  u32 start = proc->task.context.esp_save_int->esp - sizeof(regs);
  STACK_FRAME* esp_save_int_copy =
      (STACK_FRAME*)(proc->task.context.esp_save_int->esp -
                     sizeof(regs));  //: = (STACK_FRAME*)start
  *esp_save_int_copy = *proc->task.context.esp_save_int;

  // for(u32 *p = start, *sf = proc->task.context.esp_save_int, i=0;
  // i<sizeof(regs) / sizeof(u32) ; i++,p++, sf++) {
  //     __asm__ (
  //         "mov %%eax, %%es:(%%edi)"
  //         :
  //         : "a"(*sf), "D"(p)
  //         :
  //     );
  // }

  /* push para */
  start -= sizeof(Sigaction);
  Sigaction* sigaction_copy = (Sigaction*)start;
  *sigaction_copy = sigaction;

  // for(u32 *p = start, *sf = &sigaction, i = 0; i < sizeof(Sigaction) /
  // sizeof(u32) ;i++, p++, sf++) {
  //     __asm__ (
  //         "mov %%eax, %%es:(%%edi)"
  //         :
  //         : "a"(*sf), "D"(p)
  //         :
  //     );
  // }

  /* Handler return address */
  start -= sizeof(u32);

  /* switch to Handler */
  // u32 *context_p = proc->task.esp_save_int;
  proc->task.context.esp_save_int->eip = (u32)proc->task._Hanlder;
  proc->task.context.esp_save_int->esp = start;
  //*(context_p + 14) =  proc->task._Hanlder; /* eip */
  //*(context_p + 17) = start;                /* esp */

  /*  reverse  */
  __asm__ __volatile__("mov %0, %%es\n" : : "r"(A_es) :);
  enable_int();
}
