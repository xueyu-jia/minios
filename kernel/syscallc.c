#include <minios/buddy.h>
#include <minios/const.h>
#include <minios/kmalloc.h>
#include <minios/ksignal.h>
#include <minios/proc.h>
#include <minios/semaphore.h>
#include <minios/syscall.h>
#include <minios/time.h>

#define SYSCALL_ENTRY(name) [_NR_##name] = sys_##name

system_call sys_call_table[NR_SYSCALLS] = {
    SYSCALL_ENTRY(getticks),
    SYSCALL_ENTRY(getpid),
    SYSCALL_ENTRY(malloc_4k),
    SYSCALL_ENTRY(free_4k),
    SYSCALL_ENTRY(fork),
    SYSCALL_ENTRY(pthread_create),
    SYSCALL_ENTRY(execve),
    SYSCALL_ENTRY(yield),
    SYSCALL_ENTRY(sleep),
    SYSCALL_ENTRY(open),
    SYSCALL_ENTRY(close),
    SYSCALL_ENTRY(read),
    SYSCALL_ENTRY(write),
    SYSCALL_ENTRY(lseek),
    SYSCALL_ENTRY(unlink),
    SYSCALL_ENTRY(creat),
    SYSCALL_ENTRY(closedir),
    SYSCALL_ENTRY(opendir),
    SYSCALL_ENTRY(mkdir),
    SYSCALL_ENTRY(rmdir),
    SYSCALL_ENTRY(readdir),
    SYSCALL_ENTRY(chdir),
    SYSCALL_ENTRY(getcwd),
    SYSCALL_ENTRY(wait),
    SYSCALL_ENTRY(exit),
    SYSCALL_ENTRY(signal),
    SYSCALL_ENTRY(sigsend),
    SYSCALL_ENTRY(sigreturn),
    SYSCALL_ENTRY(total_mem_size),
    SYSCALL_ENTRY(shmget),
    SYSCALL_ENTRY(shmat),
    SYSCALL_ENTRY(shmdt),
    SYSCALL_ENTRY(shmctl),
    SYSCALL_ENTRY(shmmemcpy),
    SYSCALL_ENTRY(ftok),
    SYSCALL_ENTRY(msgget),
    SYSCALL_ENTRY(msgsnd),
    SYSCALL_ENTRY(msgrcv),
    SYSCALL_ENTRY(msgctl),
    SYSCALL_ENTRY(execvp),
    SYSCALL_ENTRY(execv),
    SYSCALL_ENTRY(pthread_self),
    SYSCALL_ENTRY(pthread_mutex_init),
    SYSCALL_ENTRY(pthread_mutex_destroy),
    SYSCALL_ENTRY(pthread_mutex_lock),
    SYSCALL_ENTRY(pthread_mutex_unlock),
    SYSCALL_ENTRY(pthread_mutex_trylock),
    SYSCALL_ENTRY(pthread_cond_init),
    SYSCALL_ENTRY(pthread_cond_wait),
    SYSCALL_ENTRY(pthread_cond_signal),
    SYSCALL_ENTRY(pthread_cond_timewait),
    SYSCALL_ENTRY(pthread_cond_broadcast),
    SYSCALL_ENTRY(pthread_cond_destroy),
    SYSCALL_ENTRY(getpid_by_name),
    SYSCALL_ENTRY(mount),
    SYSCALL_ENTRY(umount),
    SYSCALL_ENTRY(pthread_exit),
    SYSCALL_ENTRY(pthread_join),
    SYSCALL_ENTRY(get_time),
    SYSCALL_ENTRY(stat),
    SYSCALL_ENTRY(nice),
    SYSCALL_ENTRY(set_rt),
    SYSCALL_ENTRY(rt_prio),
    SYSCALL_ENTRY(get_proc_msg),
};

u32 do_total_mem_size() {
    return kern_total_mem_size();
}

u32 sys_total_mem_size() {
    return do_total_mem_size();
}

int do_getpid() {
    return kern_getpid();
}

int sys_getpid() {
    return do_getpid();
}

int do_getpid_by_name(char* name) {
    return kern_getpid_by_name(name);
}

int sys_getpid_by_name() {
    return do_getpid_by_name((char*)get_arg(1));
}
