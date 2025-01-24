#pragma once

#include <minios/spinlock.h>
#include <minios/cache.h>
#include <minios/protect.h>
#include <minios/x86.h>
#include <minios/types.h>
#include <minios/pthread.h>
#include <uapi/minios/signal.h>
#include <klib/stdint.h>
#include <sys/types.h>
#include <list.h>
#include <stdbool.h>

/* Used to find saved registers in the new kernel stack,
 * for there is no variable name you can use in C.
 * The macro defined below must coordinate with the equivalent in sconst.inc,
 * whose content is used in asm.
 * Note that you shouldn't use saved registers in the old
 * kernel stack, i.e. stack_frame_t, any more.
 * To access them, use a pointer plus a offset defined
 * below instead.
 * added by xw, 17/12/11
 */
#define INIT_STACK_SIZE 1024 * 8 // new kernel stack is 8kB
#define P_STACKBASE 0
#define GSREG 0
#define FSREG 1 * 4
#define ESREG 2 * 4
#define DSREG 3 * 4
#define EDIREG 4 * 4
#define ESIREG 5 * 4
#define EBPREG 6 * 4
#define KERNELESPREG 7 * 4
#define EBXREG 8 * 4
#define EDXREG 9 * 4
#define ECXREG 10 * 4
#define EAXREG 11 * 4
#define RETADR 12 * 4
#define ERROR 13 * 4 // added by lcy 2023.10.26
#define EIPREG 14 * 4
#define CSREG 15 * 4
#define EFLAGSREG 16 * 4
#define ESPREG 17 * 4
#define SSREG 18 * 4
// #define P_STACKTOP 19 * 4
#define P_STACKTOP sizeof(stack_frame_t)

/*总PCB表数和taskPCB表数*/
// modified by xw, 18/8/27
// the memory space we put kernel is 0x30400~0x6ffff, so we should limit kernel
// size
// #define NR_PCBS	32		//add by visual 2016.4.5
// #define NR_K_PCBS 10		//add by visual 2016.4.5
#define NR_PCBS 64 // modified by zhenhao 2023.3.5
// #define NR_TASKS	4	//TestA~TestC + hd_service //deleted by mingxuan
//  2019-5-19
#define NR_TASKS 3   // task_tty + hd_service		//modified by mingxuan 2019-5-19
#define NR_K_PCBS 16 // modified by zhenhao 2023.3.5
#define PID_INIT NR_K_PCBS
#define PID_NO_PROC 1000
#define PROC_READY_MAX NR_PCBS // xiaofeng // 30 -> NR_PCBS jiangfeng 24-5-5
#define PROC_NICE_MAX 19
// 初始进程为回收进程
#define NR_RECY_PROC PID_INIT

#define NR_CPUS 1   // numbers of cpu. added by xw, 18/6/1
#define NR_FILES 64 // numbers of files a process can own. added by xw, 18/6/14

enum proc_stat {
    // 空闲未分配
    FREE,
    // 正在使用且可被调度
    READY,
    // 睡眠等待状态
    SLEEPING,
    // 进程收到了一个信号，导致它被立即终止，通常是 SIGKILL 或其他无法被忽略
    // 的致命信号，被杀死的进程不会保留其 PCB 以供父进程检索状态
    KILLED,
    // 子进程已经结束运行，但其父进程尚未通过调用如 wait 或 waitpid 这样的系
    // 统调用来检索其退出状态
    ZOMBY,
};

#define NR_CHILD_MAX (NR_PCBS - NR_K_PCBS - 1)
#define TYPE_PROCESS 0
#define TYPE_THREAD 1

// 进程树记录，包括父进程，子进程，子线程
typedef struct tree_info {
    int type;                          // 当前是进程还是线程
    pid_t real_ppid;                   // 亲父进程，创建它的那个进程/线程
    pid_t ppid;                        // 当前父进程
    int child_p_num;                   // 子进程数量
    pid_t child_process[NR_CHILD_MAX]; // 子进程列表
    int child_t_num;                   // 子线程数量
    pid_t child_thread[NR_CHILD_MAX];  // 子线程列表
    bool text_hold;                    // 是否拥有代码
    bool data_hold;                    // 是否拥有数据
} tree_info_t;

struct vmem_area {
    u32 start;
    u32 end;
    u32 flags;              // both prot and flags
    struct file_desc* file; // NULL for anon region
    u32 pgoff;
    struct list_node vma_list;
};

enum {
    MEMMAP_TEXT,
    MEMMAP_DATA,
    MEMMAP_VPAGE,
    MEMMAP_SHARE,
    MEMMAP_HEAP,
    MEMMAP_STACK,
    MEMMAP_ARG,
    MEMMAP_KERNEL,
};

typedef struct {              // 线性地址分布结构体
    uintptr_t text_lin_base;  // 代码段基址
    uintptr_t text_lin_limit; // 代码段界限

    uintptr_t data_lin_base;  // 数据段基址
    uintptr_t data_lin_limit; // 数据段界限

    uintptr_t vpage_lin_base;  // 保留内存基址
    uintptr_t vpage_lin_limit; // 保留内存界限

    union {
        struct {
            uintptr_t heap_lin_base;  // 堆基址
            uintptr_t heap_lin_limit; // 堆界限
        };
        struct {
            //! NOTE: used by thread, ref to fa heap info
            uintptr_t* heap_lin_base_ref;
            uintptr_t* heap_lin_limit_ref;
        };
    };

    uintptr_t stack_lin_base;  // 栈基址
    uintptr_t stack_lin_limit; // 栈界限（使用时注意栈的生长方向）

    uintptr_t arg_lin_base;  // 参数内存基址
    uintptr_t arg_lin_limit; // 参数内存界限

    uintptr_t kernel_lin_base;  // 内核基址
    uintptr_t kernel_lin_limit; // 内核界限

    uintptr_t stack_child_limit; // 分给子线程的栈的界限
    address_space_t anon_pages;

    list_head vma_map; // list of vmem_area
    spinlock_t vma_lock;
} memmap_t;

typedef struct {
    cpu_context_t context; // 因为x86版本使用偏移量访问变量，所以请将该变量放在开头
    void* channel;         // sleep/wakeup channel
    memmap_t memmap;       // 线性内存分部信息 		add by visual 2016.5.4
    tree_info_t tree_info; // 记录进程树关系	add by visual 2016.5.25
    int ticks;             /* remained ticks */
    int priority;

    u32 pid;              /* process id passed in from MM */
    pthread_t pthread_id; // 线程id Add By ZengHao & MaLinhan 21.12.22

    char p_name[16]; /* name of the process */

    enum proc_stat stat; // add by visual 2016.4.5

    u32 cr3; // add by visual 2016.4.5

    struct dentry* cwd;

    int suspended; // 线程id

    struct file_desc* filp[NR_FILES];

    int exit_status;
    int child_exit_status;

    // 与signal相关
    void* sig_handler[NR_SIGNALS];
    u32 sig_set;
    u32 sig_arg[NR_SIGNALS];
    void* _Handler;

    // 线程相关
    pthread_attr_t attr;
    void* retval;
    u32 who_wait_flag;
    spinlock_t lock;

    // cfs attr added by xiaofeng
    int is_rt;       // flag for Real-time(T) and not-Real-time(F) process
    int rt_priority; // priority for Real-time process

    int nice;
    int weight; // priority for not-Real-time process
    double vruntime;
    u32 cpu_use;
    u64 sum_cpu_use;
} pcb_t;

typedef union {
    pcb_t task;
    char stack[INIT_STACK_SIZE];
} process_t;

typedef struct {
    fn_task_t initial_eip;
    int ready;
    int rpl;
    int rt;
    int priority_nice; // rt priority if rt == 1, else nice
    int stacksize;
    char name[32];
} task_t;

#define STACK_SIZE_TASK 0x1000

extern u32 cr3_ready;
extern process_t* p_proc_current;
extern process_t* p_proc_next;
extern process_t cpu_table[];
extern process_t proc_table[];
extern task_t task_table[NR_TASKS];

extern int nice_to_weight[];

#define proc2pid(x) (x - proc_table)
#define pid2proc(pid) ((process_t*)&proc_table[(pid)])
#define proc_kstacktop(proc) ((stack_frame_t*)(((char*)((proc) + 1) - P_STACKTOP)))
#define proc_stack_frame(proc) ((stack_frame_t*)(((char*)((proc) + 1) - P_STACKTOP)))
#define proc_context_frame(proc) \
    (context_frame_t*)((char*)proc_stack_frame(proc) - sizeof(context_frame_t))

#define PROC_CONTEXT 10 * 4

typedef struct {
    u32 pid;
    int nice;
    double vruntime;
    u64 sum_cpu_use;
} proc_msg;

#define proc_real(proc)                                                            \
    ({                                                                             \
        process_t* proc_src = (proc);                                              \
        const int ppid = proc_src->task.tree_info.ppid;                            \
        const int type = proc_src->task.tree_info.type;                            \
        process_t* proc_real = type == TYPE_THREAD ? &proc_table[ppid] : proc_src; \
        proc_real;                                                                 \
    })

static inline memmap_t* proc_memmap(process_t* p_proc) {
    return &proc_real(p_proc)->task.memmap;
}

void proc_update();

process_t* alloc_pcb();
void free_pcb(process_t* p);

int kern_getpid();
int kern_getpid_by_name(const char* name);
void kern_get_proc_msg(proc_msg* msg);

void* va2la(int pid, void* va);

void proc_backtrace();

void init_process(process_t* proc, char name[32], enum proc_stat stat, int pid, int is_rt,
                  int priority_or_nice);
void init_all_pcb();
int kthread_create(char* name, void* func, int is_rt, int privilege, int rtpriority_or_nice);
void init_user_cpu_context(cpu_context_t* context, int pid);

extern void restart_initial();
extern void restart_restore();
