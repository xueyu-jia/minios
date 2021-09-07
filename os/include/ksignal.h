//added by mingxuan 2021-2-28

#ifndef KSIGNAL_H
#define KSIGNAL_H

//#include <signal/signal.h>
#include "signal.h" //modified by mingxuan 2021-4-2

int sys_signal(void *user_esp);
int sys_sigsend(void *user_esp);
void sys_sigreturn(void *user_esp);

int do_signal(int pid, void* handler, void* _Handler);
int do_sigsend(int pid, Sigaction* sigaction) ;
void do_sigreturn(int ebp) ;

#endif 