#include <minios/syscall.h>
#include <minios/console.h>

void initial() {
    syscall(NR_execve, "/bin/init", NULL, NULL);

    //! NOTE: a kthread task can not exit or call the halt, actually we have no ideas when
    //! approached here, so simply let it trap into the dead loop
    kprintf("fatal: failed to start init process\n");
    while (1);
}
