#pragma once

#include <uapi/minios/interrupt.h>
#include <klib/stdint.h>
#include <compiler.h>
#include <macro_helper.h>

#define __ASM_ARGIN_REGID_1 MH_STRINGIFY(a)
#define __ASM_ARGIN_REGID_2 MH_STRINGIFY(b)
#define __ASM_ARGIN_REGID_3 MH_STRINGIFY(c)
#define __ASM_ARGIN_REGID_4 MH_STRINGIFY(d)
#define __ASM_ARGIN_REGID_5 MH_STRINGIFY(S)
#define __ASM_ARGIN_REGID_6 MH_STRINGIFY(D)
#define __ASM_ARGIN_REGID(i) MH_CONCAT(__ASM_ARGIN_REGID_, i)

#define __SYSCALL_ARGIN_LIST_IMPL1(N, x, ...) __ASM_ARGIN_REGID(N)(x)
#define __SYSCALL_ARGIN_LIST_IMPL2(N, x, ...) \
    __ASM_ARGIN_REGID(N)(x), __SYSCALL_ARGIN_LIST_IMPL1(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGIN_LIST_IMPL3(N, x, ...) \
    __ASM_ARGIN_REGID(N)(x), __SYSCALL_ARGIN_LIST_IMPL2(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGIN_LIST_IMPL4(N, x, ...) \
    __ASM_ARGIN_REGID(N)(x), __SYSCALL_ARGIN_LIST_IMPL3(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGIN_LIST_IMPL5(N, x, ...) \
    __ASM_ARGIN_REGID(N)(x), __SYSCALL_ARGIN_LIST_IMPL4(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGIN_LIST_IMPL6(N, x, ...) \
    __ASM_ARGIN_REGID(N)(x), __SYSCALL_ARGIN_LIST_IMPL5(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGIN_LIST_IMPL(N, ...) \
    MH_EXPAND(MH_CONCAT(__SYSCALL_ARGIN_LIST_IMPL, N)(1, __VA_ARGS__))
#define __SYSCALL_ARGIN_LIST(...) \
    MH_EXPAND(__SYSCALL_ARGIN_LIST_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))

#define SYSCALL_IMPL(retval, nr_int, nr_syscall, ...)                  \
    ({                                                                 \
        u32 retval = 0;                                                \
        asm volatile(MH_STRINGIFY(MH_CONCAT(int $, MH_EXPAND(nr_int))) \
                     : "=a"(retval)                                    \
                     : __SYSCALL_ARGIN_LIST(nr_syscall, ##__VA_ARGS__) \
                     : "cc", "memory");                                \
        retval;                                                        \
    })
#define syscall(nr_syscall, ...)                                                               \
    SYSCALL_IMPL(MH_CONCAT(MH_CONCAT(nr_syscall, _), retval), INT_VECTOR_SYS_CALL, nr_syscall, \
                 ##__VA_ARGS__)

enum {
    NR_getticks,
    NR_get_time,
    NR_getpid,
    NR_getpid_by_name,
    NR_total_mem_size,
    NR_malloc_4k,
    NR_free_4k,
    NR_fork,
    NR_exit,
    NR_execve,
    NR_wait,
    NR_sleep,
    NR_yield,
    NR_signal,
    NR_sigreturn,
    NR_sigsend,
    NR_open,
    NR_close,
    NR_read,
    NR_write,
    NR_unlink,
    NR_lseek,
    NR_creat,
    NR_mkdir,
    NR_opendir,
    NR_closedir,
    NR_readdir,
    NR_rmdir,
    NR_chdir,
    NR_getcwd,
    NR_mount,
    NR_umount,
    NR_stat,
    NR_nice,
    NR_rt_prio,
    NR_set_rt,
    NR_get_proc_msg,
    NR_ftok,
    NR_msgctl,
    NR_msgget,
    NR_msgsnd,
    NR_msgrcv,
    NR_shmctl,
    NR_shmget,
    NR_shmat,
    NR_shmdt,
    NR_shmcpy,
    NR_pthread_self,
    NR_pthread_create,
    NR_pthread_join,
    NR_pthread_exit,
    NR_pthread_cond_init,
    NR_pthread_cond_destroy,
    NR_pthread_cond_signal,
    NR_pthread_cond_broadcast,
    NR_pthread_cond_wait,
    NR_pthread_cond_timewait,
    NR_pthread_mutex_init,
    NR_pthread_mutex_destroy,
    NR_pthread_mutex_lock,
    NR_pthread_mutex_trylock,
    NR_pthread_mutex_unlock,
    NR_SYSCALLS,
};
