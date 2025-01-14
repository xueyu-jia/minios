#include <minios/proc.h>
#include <minios/sched.h>
#include <minios/asm.h>
#include <stdbool.h>

void idle() {
    while (true) {
        rq_remove(p_proc_current);
        enable_int();
        halt();
    }
}
