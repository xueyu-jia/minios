/*
#include <signal/signal.h>
#include <stdarg.h>
#include <stddef.h>
*/
#include "signal.h"
#include "syscall.h"

void Handler(Sigaction sigaction) {
  void (*_fun)(int, int);
  _fun = (void (*)(int, int))sigaction.handler;
  _fun(sigaction.sig, sigaction.arg);
  int ebp;
  __asm__ __volatile__("mov %%ebp, %0" : "=r"(ebp) : :);
  sigreturn(ebp);
}

int kill(int pid, int sig, ...) {
  va_list ap = (va_list)((char*)(&sig) + 4);
  Sigaction sigaction = {
      .sig = sig,         //信号的编号
      .handler = NULL,    // handler函数指针
      .arg = *((u32*)ap)  //传给 handler 的参数
  };
  return sigsend(pid, &sigaction);
}

int signal(int sig, void* handler) { return _signal(sig, handler, HANDLER); }