#pragma once
#include <kernel/const.h>
PUBLIC int kern_wait(int* wstatus);
PUBLIC int do_wait(int *status);
PUBLIC int sys_wait();
