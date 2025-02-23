#include <minios/proc.h>
#include <minios/sched.h>
#include <minios/interrupt.h>
#include <minios/asm.h>
#include <stdbool.h>

void idle() {
    while (true) {
        disable_int_begin();
        rq_remove(p_proc_current);
        disable_int_end();
        enable_int();
        halt();
    }
}
