#pragma once

#include <stdint.h>
#include <fcntl.h>
#include <msg.h>
#include <pthread.h>
#include <signal.h>
#include <stat.h>
#include <time.h>
#include <ushm.h>
#include <compiler.h>

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

typedef struct process_message {
    uint32_t pid;
    int nice;
    double vruntime;
    uint64_t sum_cpu_use;
} proc_msg;

int getticks();
int getpid();
void* malloc_4k();
void free_4k(void* AdddrLin);
int fork();
int pthread_create(int* thread, void* attr, void* entry, void* arg);
uint32_t execve(char* path, char* const argv[], char* const envp[]);
void yield();
void sleep(int n);
int open(const char* pathname, int flags, ...);
int close(int fd);
int read(int fd, void* buf, int count);
int write(int fd, const void* buf, int count);
int lseek(int fd, int offset, int whence);
int unlink(const char* pathname);
int creat(const char* pathname);
DIR* opendir(const char* dirname);
int closedir(DIR* dirp);
int mkdir(const char* dirname, int mode);
int rmdir(const char* dirname);
struct dirent* readdir(DIR* dirp);
int chdir(const char* path);
char* getcwd(char* buf, int size);
int wait(int* status);
NORETURN void exit(int status);
int _signal(int sig, void* handler,
            void* _Handler); //"user/ulib/signal.c" 中提供了上层封装
int sigsend(int pid, Sigaction* sigaction_p);
void sigreturn(int ebp);
uint32_t total_mem_size();
int shmget(int key, int size, int shmflg);
void* _shmat(int shmid, char* shmaddr,
             int shmflg);   //"user/ulib/ushm.c" 中提供了上层封装
void _shmdt(char* shmaddr); //"user/ulib/ushm.c" 中提供了上层封装
struct ipc_shm* shmctl(int shmid, int cmd, struct ipc_shm* buf);
void* shmmemcpy(void* dst, const void* src, long unsigned int len);
int ftok(char* f, int key);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int msgctl(int msgqid, int cmd, msqid_ds* buf);
void test(int no);
uint32_t execvp(char* file, char* argv[]);
uint32_t execv(char* path, char* argv[]);
pthread_t pthread_self();
int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* mutexattr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_destroy(pthread_cond_t* cond);
int getpid_by_name(char* name);
int mount(const char* source, const char* target, const char* filesystemtype,
          unsigned long mountflags, const void* data);
int umount(const char* target);
void pthread_exit(void* retval);
int pthread_join(pthread_t pthread, void** retval);
int get_time(struct tm* time);
int stat(const char* pathname, struct stat* statbuf);
void nice(int val);
void set_rt(int turn_rt);
void rt_prio(int prio);
void get_proc_msg(proc_msg* msg);
