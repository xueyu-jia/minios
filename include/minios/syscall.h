#pragma once

enum {
    _NR_getticks,
    _NR_getpid,
    _NR_malloc_4k,
    _NR_free_4k,
    _NR_fork,
    _NR_pthread_create,
    _NR_execve,
    _NR_yield,
    _NR_sleep,
    _NR_open,
    _NR_close,
    _NR_read,
    _NR_write,
    _NR_lseek,
    _NR_unlink,
    _NR_creat,
    _NR_closedir,
    _NR_opendir,
    _NR_mkdir,
    _NR_rmdir,
    _NR_readdir,
    _NR_chdir,
    _NR_getcwd,
    _NR_wait,
    _NR_exit,
    _NR_signal,
    _NR_sigsend,
    _NR_sigreturn,
    _NR_total_mem_size,
    _NR_shmget,
    _NR_shmat,
    _NR_shmdt,
    _NR_shmctl,
    _NR_shmmemcpy,
    _NR_ftok,
    _NR_msgget,
    _NR_msgsnd,
    _NR_msgrcv,
    _NR_msgctl,
    _NR_test,
    _NR_execvp,
    _NR_execv,
    _NR_pthread_self,
    _NR_pthread_mutex_init,
    _NR_pthread_mutex_destroy,
    _NR_pthread_mutex_lock,
    _NR_pthread_mutex_unlock,
    _NR_pthread_mutex_trylock,
    _NR_pthread_cond_init,
    _NR_pthread_cond_wait,
    _NR_pthread_cond_signal,
    _NR_pthread_cond_timewait,
    _NR_pthread_cond_broadcast,
    _NR_pthread_cond_destroy,
    _NR_getpid_by_name,
    _NR_mount,
    _NR_umount,
    _NR_pthread_exit,
    _NR_pthread_join,
    _NR_get_time,
    _NR_stat,
    _NR_nice,
    _NR_set_rt,
    _NR_rt_prio,
    _NR_get_proc_msg,
    NR_SYSCALLS,
};

#define INT_VECTOR_SYS_CALL 0x90

/* 无参数的系统调用 */
#define _syscall0(NR_syscall)                                                         \
    ({                                                                                \
        int _retval;                                                                  \
        asm volatile("int $0x90" : "=a"(_retval) : "a"(NR_syscall) : "cc", "memory"); \
        _retval;                                                                      \
    })

/* 一个参数的系统调用 */
#define _syscall1(NR_syscall, ARG1)                                                              \
    ({                                                                                           \
        int _retval;                                                                             \
        asm volatile("int $0x90" : "=a"(_retval) : "a"(NR_syscall), "b"(ARG1) : "cc", "memory"); \
        _retval;                                                                                 \
    })

/* 两个参数的系统调用 */
#define _syscall2(NR_syscall, ARG1, ARG2)                    \
    ({                                                       \
        int _retval;                                         \
        asm volatile("int $0x90"                             \
                     : "=a"(_retval)                         \
                     : "a"(NR_syscall), "b"(ARG1), "c"(ARG2) \
                     : "cc", "memory");                      \
        _retval;                                             \
    })

/* 三个参数的系统调用 */
#define _syscall3(NR_syscall, ARG1, ARG2, ARG3)                         \
    ({                                                                  \
        int _retval;                                                    \
        asm volatile("int $0x90"                                        \
                     : "=a"(_retval)                                    \
                     : "a"(NR_syscall), "b"(ARG1), "c"(ARG2), "d"(ARG3) \
                     : "cc", "memory");                                 \
        _retval;                                                        \
    })

/* 四个参数的系统调用 */
#define _syscall4(NR_syscall, ARG1, ARG2, ARG3, ARG4)                              \
    ({                                                                             \
        int _retval;                                                               \
        asm volatile("int $0x90"                                                   \
                     : "=a"(_retval)                                               \
                     : "a"(NR_syscall), "b"(ARG1), "c"(ARG2), "d"(ARG3), "S"(ARG4) \
                     : "cc", "memory");                                            \
        _retval;                                                                   \
    })

/* 五个参数的系统调用 */
#define _syscall5(NR_syscall, ARG1, ARG2, ARG3, ARG4, ARG5)                                   \
    ({                                                                                        \
        int _retval;                                                                          \
        asm volatile("int $0x90"                                                              \
                     : "=a"(_retval)                                                          \
                     : "a"(NR_syscall), "b"(ARG1), "c"(ARG2), "d"(ARG3), "S"(ARG4), "D"(ARG5) \
                     : "cc", "memory");                                                       \
        _retval;                                                                              \
    })
