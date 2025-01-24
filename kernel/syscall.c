#include <minios/syscall.h>
#include <minios/asm.h>
#include <stdbool.h>
#include <macro_helper.h>

#define __SYSCALL_ARGOUT_LIST_IMPL0(...)
#define __SYSCALL_ARGOUT_LIST_IMPL1(N, type, ...) ((type)syscall_get_arg(N))
#define __SYSCALL_ARGOUT_LIST_IMPL2(N, type, ...) \
    ((type)syscall_get_arg(N)), __SYSCALL_ARGOUT_LIST_IMPL1(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGOUT_LIST_IMPL3(N, type, ...) \
    ((type)syscall_get_arg(N)), __SYSCALL_ARGOUT_LIST_IMPL2(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGOUT_LIST_IMPL4(N, type, ...) \
    ((type)syscall_get_arg(N)), __SYSCALL_ARGOUT_LIST_IMPL3(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGOUT_LIST_IMPL5(N, type, ...) \
    ((type)syscall_get_arg(N)), __SYSCALL_ARGOUT_LIST_IMPL4(MH_NEXT_INT(N), __VA_ARGS__)
#define __SYSCALL_ARGOUT_LIST_IMPL(N, ...) \
    MH_EXPAND(MH_CONCAT(__SYSCALL_ARGOUT_LIST_IMPL, N)(0, __VA_ARGS__))
#define __SYSCALL_ARGOUT_LIST(...) \
    MH_EXPAND(__SYSCALL_ARGOUT_LIST_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))

#define SYSCALL_IMPL_MAKE_PARAM(index, type) MH_COMMA type MH_CONCAT(_, index)
#define SYSCALL_IMPL_MAKE_ARGIN(index, ...) MH_COMMA MH_CONCAT(_, index)
#define SYSCALL_IMPL_MAKE_ARGSEQ_BUILD(type, ...) \
    dummy MH_FOREACH_PRED2_ITER(MH_NEXT_INT, MH_CONCAT(SYSCALL_IMPL_MAKE_, type), 1, __VA_ARGS__)
#define SYSCALL_IMPL_MAKE_ARGSEQ(type, ...) \
    MH_INVOKE(MH_CDR, SYSCALL_IMPL_MAKE_ARGSEQ_BUILD(type, __VA_ARGS__))

#define SYSCALL_IMPL_PROTO(name, ret_type, ...) \
    ret_type MH_CONCAT(do_, name)(SYSCALL_IMPL_MAKE_ARGSEQ(PARAM, __VA_ARGS__))
#define SYSCALL_IMPL_ARGLIST(...) SYSCALL_IMPL_MAKE_ARGSEQ(ARGIN, __VA_ARGS__)

#define SYSCALL_IMPL_RET_AS(name, ret_type, ...) SYSCALL_IMPL_PROTO(name, ret_type, __VA_ARGS__)
#define SYSCALL_IMPL_NORET_AS(name, ...) SYSCALL_IMPL_PROTO(name, void, __VA_ARGS__)

#define SYSCALL_IMPL_RET(name, ret_type, ...)                             \
    SYSCALL_IMPL_RET_AS(name, ret_type, __VA_ARGS__) {                    \
        return MH_CONCAT(kern_, name)(SYSCALL_IMPL_ARGLIST(__VA_ARGS__)); \
    }

#define SYSCALL_IMPL_NORET(name, ...)                              \
    SYSCALL_IMPL_NORET_AS(name, __VA_ARGS__) {                     \
        MH_CONCAT(kern_, name)(SYSCALL_IMPL_ARGLIST(__VA_ARGS__)); \
    }

#define DECLARE_SYSCALL(name, ret_type, ...)                                       \
    SYSCALL_IMPL_RET(name, ret_type, __VA_ARGS__)                                  \
    static ret_type MH_CONCAT(sys_, name)() {                                      \
        return (ret_type)MH_CONCAT(do_, name)(__SYSCALL_ARGOUT_LIST(__VA_ARGS__)); \
    }

#define DECLARE_SYSCALL_NORET(name, ...)                          \
    SYSCALL_IMPL_NORET(name, __VA_ARGS__)                         \
    static u32 MH_CONCAT(sys_, name)() {                          \
        MH_CONCAT(do_, name)(__SYSCALL_ARGOUT_LIST(__VA_ARGS__)); \
        return 0;                                                 \
    }

#define DECLARE_SYSCALL_AS(name, ret_type, ...)                                    \
    SYSCALL_IMPL_PROTO(name, ret_type, __VA_ARGS__);                               \
    static ret_type MH_CONCAT(sys_, name)() {                                      \
        return (ret_type)MH_CONCAT(do_, name)(__SYSCALL_ARGOUT_LIST(__VA_ARGS__)); \
    }                                                                              \
    SYSCALL_IMPL_RET_AS(name, ret_type, __VA_ARGS__)

#define DECLARE_SYSCALL_NORET_AS(name, ...)                       \
    SYSCALL_IMPL_PROTO(name, void, __VA_ARGS__);                  \
    static u32 MH_CONCAT(sys_, name)() {                          \
        MH_CONCAT(do_, name)(__SYSCALL_ARGOUT_LIST(__VA_ARGS__)); \
        return 0;                                                 \
    }                                                             \
    SYSCALL_IMPL_NORET_AS(name, __VA_ARGS__)

u32 syscall_get_arg(int index) {
    stack_frame_t* frame = (void*)p_proc_current->task.context.esp_save_int;
    switch (index) {
        case 0: {
            return frame->ebx;
        } break;
        case 1: {
            return frame->ecx;
        } break;
        case 2: {
            return frame->edx;
        } break;
        case 3: {
            return frame->esi;
        } break;
        case 4: {
            return frame->edi;
        } break;
        case 5: {
            return frame->ebp;
        } break;
    }
    while (true) { halt(); }
}

DECLARE_SYSCALL(getticks, int);
DECLARE_SYSCALL(get_time, int, struct tm*);
DECLARE_SYSCALL(getpid, int);
DECLARE_SYSCALL(getpid_by_name, int, const char*);
DECLARE_SYSCALL(total_mem_size, u32);
DECLARE_SYSCALL(malloc_4k, u32);
DECLARE_SYSCALL_NORET(free_4k, void*);
DECLARE_SYSCALL(fork, int);
DECLARE_SYSCALL_NORET(exit, int);
DECLARE_SYSCALL(execve, int, const char*, char* const*, char* const*);
DECLARE_SYSCALL(wait, int, int*);
DECLARE_SYSCALL_NORET(sleep, int);
DECLARE_SYSCALL_NORET(yield);
DECLARE_SYSCALL(signal, int, int, void*, void*);
DECLARE_SYSCALL_NORET(sigreturn, u32);
DECLARE_SYSCALL(sigsend, int, int, Sigaction*);
DECLARE_SYSCALL_AS(open, int, const char*, int, int) {
    return kern_vfs_open(_1, _2, _3);
}
DECLARE_SYSCALL_AS(close, int, int) {
    return kern_vfs_close(_1);
}
DECLARE_SYSCALL_AS(read, int, int, void*, int) {
    return kern_vfs_read(_1, _2, _3);
}
DECLARE_SYSCALL_AS(write, int, int, const void*, int) {
    return kern_vfs_write(_1, _2, _3);
}
DECLARE_SYSCALL_AS(unlink, int, const char*) {
    return kern_vfs_unlink(_1);
}
DECLARE_SYSCALL_AS(lseek, int, int, int, int) {
    return kern_vfs_lseek(_1, _2, _3);
}
DECLARE_SYSCALL_AS(mkdir, int, const char*, int) {
    return kern_vfs_mkdir(_1, _2);
}
DECLARE_SYSCALL_AS(opendir, DIR*, const char*) {
    return kern_vfs_opendir(_1);
}
DECLARE_SYSCALL_AS(closedir, int, DIR*) {
    return kern_vfs_closedir(_1);
}
DECLARE_SYSCALL_AS(readdir, struct dirent*, DIR*) {
    return kern_vfs_readdir(_1);
}
DECLARE_SYSCALL_AS(rmdir, int, const char*) {
    return kern_vfs_rmdir(_1);
}
DECLARE_SYSCALL_AS(chdir, int, const char*) {
    return kern_vfs_chdir(_1);
}
DECLARE_SYSCALL_AS(getcwd, char*, char*, int) {
    return kern_vfs_getcwd(_1, _2);
}
DECLARE_SYSCALL_AS(mount, int, const char*, const char*, const char*, int, const void*) {
    return kern_vfs_mount(_1, _2, _3, _4, _5);
}
DECLARE_SYSCALL_AS(umount, int, const char*) {
    return kern_vfs_umount(_1);
}
DECLARE_SYSCALL_AS(stat, int, const char*, struct stat*) {
    return kern_vfs_stat(_1, _2);
}
DECLARE_SYSCALL_NORET(nice, int);
DECLARE_SYSCALL_NORET(rt_prio, int);
DECLARE_SYSCALL_NORET(set_rt, bool);
DECLARE_SYSCALL_NORET(get_proc_msg, proc_msg*);
DECLARE_SYSCALL(ftok, int, const char*, int);
DECLARE_SYSCALL(msgctl, int, int, int, msqid_ds_t*);
DECLARE_SYSCALL(msgget, int, key_t, int);
DECLARE_SYSCALL(msgsnd, int, int, const void*, int, int);
DECLARE_SYSCALL(msgrcv, int, int, void*, int, long, int);
DECLARE_SYSCALL(shmctl, shm_t*, int, int, shm_t*);
DECLARE_SYSCALL(shmget, int, int, int, int);
DECLARE_SYSCALL(shmat, void*, int, const void*, int);
DECLARE_SYSCALL(shmdt, int, const void*);
DECLARE_SYSCALL_NORET(shmcpy, void*, const void*, size_t);
DECLARE_SYSCALL(pthread_self, pthread_t);
DECLARE_SYSCALL_AS(pthread_create, int, pthread_t*, const pthread_attr_t*, pthread_entry_t, void*) {
    extern int kern_pthread_create_internal(pthread_t * thread, const pthread_attr_t* attr,
                                            pthread_entry_t wrapped_start_routine, void* arg);
    return kern_pthread_create_internal(_1, _2, _3, _4);
}
DECLARE_SYSCALL(pthread_join, int, pthread_t, void**);
DECLARE_SYSCALL_NORET(pthread_exit, void*);
DECLARE_SYSCALL(pthread_cond_init, int, pthread_cond_t*, const pthread_condattr_t*);
DECLARE_SYSCALL(pthread_cond_destroy, int, pthread_cond_t*);
DECLARE_SYSCALL(pthread_cond_signal, int, pthread_cond_t*);
DECLARE_SYSCALL(pthread_cond_broadcast, int, pthread_cond_t*);
DECLARE_SYSCALL(pthread_cond_wait, int, pthread_cond_t*, pthread_mutex_t*);
DECLARE_SYSCALL(pthread_cond_timewait, int, pthread_cond_t*, pthread_mutex_t*, int*);
DECLARE_SYSCALL(pthread_mutex_init, int, pthread_mutex_t*, pthread_mutexattr_t*);
DECLARE_SYSCALL(pthread_mutex_destroy, int, pthread_mutex_t*);
DECLARE_SYSCALL(pthread_mutex_lock, int, pthread_mutex_t*);
DECLARE_SYSCALL(pthread_mutex_trylock, int, pthread_mutex_t*);
DECLARE_SYSCALL(pthread_mutex_unlock, int, pthread_mutex_t*);
DECLARE_SYSCALL(pthread_rwlock_init, int, pthread_rwlock_t*, const pthread_rwlockattr_t*);
DECLARE_SYSCALL(pthread_rwlock_destroy, int, pthread_rwlock_t*);
DECLARE_SYSCALL(pthread_rwlock_rdlock, int, pthread_rwlock_t*);
DECLARE_SYSCALL(pthread_rwlock_tryrdlock, int, pthread_rwlock_t*);
DECLARE_SYSCALL(pthread_rwlock_wrlock, int, pthread_rwlock_t*);
DECLARE_SYSCALL(pthread_rwlock_trywrlock, int, pthread_rwlock_t*);
DECLARE_SYSCALL(pthread_rwlock_unlock, int, pthread_rwlock_t*);

#define SYSCALL_ENTRY_COMPLETED
#include "syscall_table.inl"
