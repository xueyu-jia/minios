#pragma once

#include <uapi/minios/syscall.h>

u32 syscall_get_arg(int index);

#include <minios/clock.h>
#include <minios/time.h>
#include <minios/proc.h>
#include <minios/memman.h>
#include <minios/fork.h>
#include <minios/exit.h>
#include <minios/exec.h>
#include <minios/wait.h>
#include <minios/sched.h>
#include <minios/signal.h>
#include <minios/vfs.h>
#include <minios/msg.h>
#include <minios/shm.h>
#include <minios/pthread.h>

int do_getticks();
clock_t do_clock();
int do_get_time(struct tm* time);
int do_getpid();
int do_getpid_by_name(const char* name);
u32 do_total_mem_size();
u32 do_malloc_4k();
void do_free_4k(void* va);
int do_fork();
void do_exit(int status);
int do_execve(const char* path, char* const* argv, char* const* envp);
int do_wait(int* wstatus);
void do_sleep(int ms);
void do_yield();
int do_signal(int sig, void* handler, void* _Handler);
void do_sigreturn(u32 ebp);
int do_sigsend(int pid, Sigaction* action);
int do_open(const char* path, int flags, int mode);
int do_close(int fd);
int do_read(int fd, void* buf, int count);
int do_write(int fd, const void* buf, int count);
int do_unlink(const char* path);
int do_lseek(int fd, int offset, int whence);
int do_creat(const char* path);
int do_mkdir(const char* path, int mode);
DIR* do_opendir(const char* path);
int do_closedir(DIR* dir);
struct dirent* do_readdir(DIR* dir);
int do_rmdir(const char* path);
int do_chdir(const char* path);
char* do_getcwd(char* buf, int size);
int do_mount(const char* src, const char* dst, const char* fs_type, int flags, const void* data);
int do_umount(const char* dst);
int do_stat(const char* path, struct stat* stat_buf);
void do_nice(int inc);
void do_rt_prio(int prio);
void do_set_rt(bool is_realtime);
void do_get_proc_msg(proc_msg* msg);
int do_ftok(const char* pathname, int proj_id);
int do_msgctl(int mq_id, int cmd, msqid_ds_t* buf);
int do_msgget(key_t key, int flags);
int do_msgsnd(int mq_id, const void* msg, int size, int flags);
int do_msgrcv(int mq_id, void* msg, int size, long type, int flags);
shm_t* do_shmctl(int shmid, int cmd, shm_t* buf);
int do_shmget(int shmid, int size, int flags);
void* do_shmat(int shmid, const void* addr, int flags);
int do_shmdt(const void* addr);
void do_shmcpy(void* dst, const void* src, size_t size);
pthread_t do_pthread_self();
int do_pthread_create(pthread_t* thread, const pthread_attr_t* attr, pthread_entry_t start_routine,
                      void* arg);
int do_pthread_join(pthread_t thread, void** retval);
void do_pthread_exit(void* retval);
int do_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int do_pthread_cond_destroy(pthread_cond_t* cond);
int do_pthread_cond_signal(pthread_cond_t* cond);
int do_pthread_cond_broadcast(pthread_cond_t* cond);
int do_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int do_pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout);
int do_pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr);
int do_pthread_mutex_destroy(pthread_mutex_t* mutex);
int do_pthread_mutex_lock(pthread_mutex_t* mutex);
int do_pthread_mutex_trylock(pthread_mutex_t* mutex);
int do_pthread_mutex_unlock(pthread_mutex_t* mutex);
int do_pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr);
int do_pthread_rwlock_destroy(pthread_rwlock_t* rwlock);
int do_pthread_rwlock_rdlock(pthread_rwlock_t* rwlock);
int do_pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock);
int do_pthread_rwlock_wrlock(pthread_rwlock_t* rwlock);
int do_pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock);
int do_pthread_rwlock_unlock(pthread_rwlock_t* rwlock);
