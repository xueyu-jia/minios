#include <stdlib.h>
#include <sys/syscall.h>

int nice(int incr) {
    syscall(NR_nice, incr);
    return 0;
}

void yield() {
    syscall(NR_yield);
}

void sleep(int ms) {
    syscall(NR_sleep, ms);
}

void set_rt(bool is_realtime) {
    syscall(NR_set_rt, is_realtime);
}

void rt_prio(int prio) {
    syscall(NR_rt_prio, prio);
}

void get_proc_msg(proc_msg_t* msg) {
    syscall(NR_get_proc_msg, msg);
}
