#pragma once
#include "const.h"
PUBLIC int kern_wait(int* wstatus);
PUBLIC int do_wait(int *status);
PUBLIC int sys_wait();
