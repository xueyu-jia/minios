// added by mingxuan 2021-2-28

#ifndef KSIGNAL_H
#define KSIGNAL_H

#include <kernel/signal.h>  //modified by mingxuan 2021-4-2

int sys_signal();
int sys_sigsend();
void sys_sigreturn();

int do_signal(int pid, void* handler, void* _Handler);
int do_sigsend(int pid, Sigaction* sigaction);
void do_sigreturn(int ebp);

#endif
