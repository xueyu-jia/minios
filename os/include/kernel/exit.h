#pragma once
#include <kernel/const.h>
PUBLIC void do_exit(int status);
PUBLIC void sys_exit();
PUBLIC void kern_exit(int exit_code);
