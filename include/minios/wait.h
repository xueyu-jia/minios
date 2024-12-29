#pragma once
#include <minios/const.h>
int kern_wait(int* wstatus);
int do_wait(int* status);
int sys_wait();
