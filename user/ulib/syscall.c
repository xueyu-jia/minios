#include "../include/syscall.h"

int get_ticks() {
	return _syscall0(_NR_get_ticks);
}

int get_pid() {
	return _syscall0(_NR_get_pid);
}
void* malloc_4k() {
	return _syscall0(_NR_malloc_4k);
}

int free_4k(void* AdddrLin) {
	return _syscall1(_NR_free_4k, AdddrLin);
}

int fork() {
	return _syscall0(_NR_fork);
}

int pthread_create(int* thread, void* attr, void* entry, void* arg) {
	return _syscall4(_NR_pthread_create, thread, attr, entry, arg);
}

void udisp_int(int arg) {
	_syscall1(_NR_udisp_int, arg);
}

void udisp_str(char* arg) {
	_syscall1(_NR_udisp_str, arg);
}

u32 execve(char* path, char* argv[], char* envp[]) {
	return _syscall3(_NR_execve, path, argv, envp);
}

void yield() {
	_syscall0(_NR_yield);
}

void sleep(int n) {
	return _syscall1(_NR_sleep, n);
}

int open(const char* pathname, int flags) {
	return _syscall2(_NR_open, pathname, flags);
}

int close(int fd) {
	return _syscall1(_NR_close, fd);
}

int read(int fd, void* buf, int count) {
	return _syscall3(_NR_read, fd, buf, count);
}

int write(int fd, const void* buf, int count) {
	return _syscall3(_NR_write, fd, buf, count);
}

int lseek(int fd, int offset, int whence) {
	return _syscall3(_NR_lseek, fd, offset, whence);
}

int unlink(const char* pathname) {
	return _syscall1(_NR_unlink, pathname);
}

int create(char* pathname) {
	return _syscall1(_NR_create, pathname);
}

int delete(const char* pathname) {
	return _syscall1(_NR_delete, pathname);
}

int opendir(const char* dirname) {
	return _syscall1(_NR_opendir, dirname);
}

int createdir(const char* dirname) {
	return _syscall1(_NR_createdir, dirname);
}

int deletedir(const char* dirname) {
	return _syscall1(_NR_deletedir, dirname);
}

int readdir(const char* dirname, unsigned int dir[3], char* filename) {
	return _syscall3(_NR_readdir, dirname, dir, filename);
}

int chdir(const char* path) {
	return _syscall1(_NR_chdir, path);
}

char* getcwd(char* buf, int size) {
	return _syscall2(_NR_getcwd, buf, size);
}

int wait(int *status) {
	return _syscall1(_NR_wait, status);
}

void exit(int status) {
	_syscall1(_NR_exit, status);
}

// "user/ulib/signal.c" 中提供了上层封装
int _signal(int sig, void* handler, void* _Handler) {
	return _syscall3(_NR_signal, sig, handler, _Handler);
}

int sigsend(int pid, Sigaction* sigaction_p) {
	return _syscall2(_NR_sigsend, pid, sigaction_p);
}

void sigreturn(int ebp) {
	_syscall1(_NR_sigreturn, ebp);
}

u32 total_mem_size() {
	return _syscall0(_NR_total_mem_size);
}

int shmget(int key, int size, int shmflg) {
	return _syscall3(_NR_shmget, key, size, shmflg);
}

// "user/ulib/ushm.c" 中提供了上层封装
void* _shmat(int shmid, char* shmaddr, int shmflg) {
	return _syscall3(_NR_shmat, shmid, shmaddr, shmflg);
}

// "user/ulib/ushm.c" 中提供了上层封装
void _shmdt(char* shmaddr) {
	_syscall1(_NR_shmdt, shmaddr);
}

struct ipc_shm* shmctl(int shmid, int cmd, struct ipc_shm* buf) {
	return _syscall3(_NR_shmctl, shmid, cmd, buf);
}

void* shmmemcpy(void* dst, const void* src, long unsigned int len) {
	return _syscall3(_NR_shmmemcpy, dst, src, len);
}

int ftok(char* f,int key) {
	return _syscall2(_NR_ftok, f, key);
}

int msgget(key_t key, int msgflg) {
	return _syscall2(_NR_msgget, key, msgflg);
}

int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg) {
	return _syscall4(_NR_msgsnd, msqid, msgp, msgsz, msgflg);
}

int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg) {
	return _syscall5(_NR_msgrcv, msqid, msgp, msgsz, msgtyp, msgflg);
}

int msgctl(int msgqid, int cmd, msqid_ds* buf) {
	return _syscall3(_NR_msgctl, msgqid, cmd, buf);
}

void test(int no) {
	_syscall1(_NR_test, no);
}

u32 execvp(char* file, char* argv[]) {
	return _syscall2(_NR_execvp, file, argv);
}

u32 execv(char* path, char* argv[]) {
	return _syscall2(_NR_execv, path, argv);
}

pthread_t pthread_self() {
	return _syscall0(_NR_pthread_self);
}

int pthread_mutex_init (pthread_mutex_t* mutex, pthread_mutexattr_t* mutexattr) {
	return _syscall2(_NR_pthread_mutex_init, mutex, mutexattr);
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
	return _syscall1(_NR_pthread_mutex_destroy, mutex);
}

int pthread_mutex_lock (pthread_mutex_t* mutex) {
	return _syscall1(_NR_pthread_mutex_lock, mutex);
}

int pthread_mutex_unlock (pthread_mutex_t* mutex) {
	return _syscall1(_NR_pthread_mutex_unlock, mutex);
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
	return _syscall1(_NR_pthread_mutex_trylock, mutex);
}

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr) {
	return _syscall2(_NR_pthread_cond_init, cond, cond_attr);
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
	return _syscall2(_NR_pthread_cond_wait, cond, mutex);
}

int pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout) {
	return _syscall3(_NR_pthread_cond_timewait, cond, mutex, timeout);
}

int pthread_cond_signal(pthread_cond_t* cond) {
	return _syscall1(_NR_pthread_cond_signal, cond);
}

int pthread_cond_broadcast(pthread_cond_t* cond) {
	return _syscall1(_NR_pthread_cond_broadcast, cond);
}

int pthread_cond_destroy(pthread_cond_t* cond) {
	return _syscall1(_NR_pthread_cond_destroy, cond);
}

int get_pid_byname(char* name) {
	return _syscall1(_NR_get_pid_byname, name);
}

int mount(const char *source, const char *target,const char *filesystemtype, 
			unsigned long mountflags, const void *data) {
	return _syscall5(_NR_mount, source, target, filesystemtype, mountflags, data);
}

int umount(const char *target) {
	return _syscall1(_NR_umount, target);
}

int init_block_dev(int drive) {
	return _syscall1(_NR_init_block_dev, drive);
}

void pthread_exit(void *retval){
	return _syscall1(_NR_pthread_exit, retval);
}

int pthread_join(pthread_t thread, void **retval){
	return _syscall2(_NR_pthread_join, thread, retval);
}