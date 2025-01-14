#include <minios/pthread.h>
#include <minios/memman.h>
#include <minios/mmap.h>
#include <minios/proc.h>
#include <minios/console.h>
#include <minios/interrupt.h>
#include <minios/pthread.h>
#include <minios/sched.h>
#include <fs/fs.h>
#include <klib/size.h>
#include <string.h>

static int pthread_pcb_cpy(process_t *p_child, process_t *p_parent);
static int pthread_update_info(process_t *p_child, process_t *p_parent);
static int pthread_stack_init(process_t *p_child, process_t *p_parent, pthread_attr_t *attr);
static int pthread_heap_init(process_t *p_child, process_t *p_parent);

int kern_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                        pthread_entry_t start_routine, void *arg) {
    process_t *p_child;
    process_t *p_parent;

    p_parent = proc_real(p_proc_current);

    pthread_attr_t th_attr = {};
    if (attr == NULL) {
        th_attr.detachstate = PTHREAD_CREATE_JOINABLE;
        th_attr.schedpolicy = SCHED_FIFO;
        th_attr.inheritsched = PTHREAD_INHERIT_SCHED;
        th_attr.scope = PTHREAD_SCOPE_PROCESS;
        th_attr.guardsize = 0;
        th_attr.stackaddr_set = 0;
        th_attr.stacksize = SZ_16K;
        th_attr.stackaddr = (void *)(p_parent->task.memmap.stack_child_limit + th_attr.stacksize);
    } else {
        th_attr = *attr;
    }

    /*****************申请空白PCB表**********************/
    p_child = alloc_pcb();
    if (0 == p_child) {
        kprintf("PCB NULL,pthread faild!");
        return -1;
    } else {
        spinlock_lock_or_yield(&p_parent->task.lock);
        /************复制父进程的PCB部分内容（保留了自己的标识信息,但cr3使用的是父进程的）**************/
        pthread_pcb_cpy(p_child, p_parent);
        spinlock_release(&p_child->task.lock);
        pthread_update_info(p_child, p_parent); // 此步骤先完成，mmap由此直接找到父进程的memmap
        pthread_stack_init(p_child, p_parent, &th_attr);
        /**************初始化子线程的堆（此时的这两个变量已经变成了指针）***********************/
        pthread_heap_init(p_child, p_parent);

        //! TODO: ensures the thread terminates with exit or pthread_exit
        stack_frame_t *p_reg = proc_kstacktop(p_child);
        p_reg->eip = (u32)start_routine;
        p_reg->esp = p_child->task.memmap.stack_lin_base;

        /**************修改子线程的线程属性************************/
        p_child->task.attr = th_attr;
        p_child->task.who_wait_flag = -1;

        /************修改子进程的名字***************/
        strcpy(p_child->task.p_name, "pthread"); // 所有的子进程都叫pthread

        p_reg->eax = 0;
        /*************子进程参数***************/
        u32 reg_esp = p_reg->esp;
        reg_esp -= SZ_4;
        *((u32 *)reg_esp) = (u32)arg;
        // *((u32*)(p_reg + ESPREG - P_STACKTOP)) -= SZ_4*2;
        p_reg->esp -= SZ_4 * 2;

        p_child->task.stat = READY;
        p_child->task.pthread_id = p_parent->task.tree_info.child_t_num + 1;
        rq_insert(p_child);
        spinlock_release(&p_child->task.lock);
        spinlock_release(&p_parent->task.lock);
    }
    *thread = p_child->task.pid;
    return 0;
}

/**********************************************************
 *		pthread_pcb_cpy			//add by visual 2016.5.26
 *复制父进程PCB表，但是又马上恢复了子进程的标识信息
 *************************************************************/
static int pthread_pcb_cpy(process_t *p_child, process_t *p_parent) {
    u32 eflags, cr3_child;
    UNUSED(eflags);
    UNUSED(cr3_child);

    int pid = p_child->task.pid;

    // 复制PCB内容
    //  esp_save_int and esp_save_context must be saved, because the child and
    //  the parent use different kernel stack! And these two are importent to
    //  the child's initial running. Added by xw, 18/4/21
    stack_frame_t *esp_save_int = p_child->task.context.esp_save_int;
    context_frame_t *esp_save_context = p_child->task.context.esp_save_context;

    UNUSED(esp_save_int);
    UNUSED(esp_save_context);

    disable_int_begin();

    p_child->task = p_parent->task;

    //! NOTE: signal parts are compelety shared with the main thread, i.e. real
    //! proc, we do not need to maintain them in child's pcb, however, handlers
    //! can be kept for convenient use
    p_child->task.sig_set = 0;

    // note that syscalls can be interrupted now! the state of child can only be
    // setted READY when anything else is well prepared. if an interruption
    // happens right here, an error will still occur. fixed jiangfeng , disable
    // int to protect 2024.4
    p_child->task.stat = SLEEPING; // 统一PCB state jiangfeng 20240314
    disable_int_end();

    // 线程使用父进程的mmap 链表，此两项直接重置为空
    init_mem_page(&p_child->task.memmap.anon_pages, MEMPAGE_AUTO);
    list_init(&p_child->task.memmap.vma_map);
    // p_child->task.esp_save_context = esp_save_context;	//same above
    memcpy((char *)proc_kstacktop(p_child), proc_kstacktop(p_parent), P_STACKTOP);
    p_child->task.pid = pid;
    init_user_cpu_context(&p_child->task.context, p_child->task.pid);

    return 0;
}

/**********************************************************
 *		pthread_update_info			//add by visual
 *2016.5.26 更新父进程和子线程程的进程树标识info
 *************************************************************/
static int pthread_update_info(process_t *p_child, process_t *p_parent) {
    /************更新父进程的info***************/ // 注意 父进程 父进程 父进程
    if (p_parent != p_proc_current) { // 只有在线程创建线程的时候才会执行
                                      // ，p_parent事实上是父进程
        p_parent->task.tree_info.child_t_num += 1; // 子线程数量
        p_parent->task.tree_info.child_thread[p_parent->task.tree_info.child_t_num - 1] =
            p_child->task.pid; // 子线程列表
    }
    /************更新父线程的info**************/
    p_proc_current->task.tree_info.child_t_num += 1; // 子线程数量
    p_proc_current->task.tree_info.child_thread[p_proc_current->task.tree_info.child_t_num - 1] =
        p_child->task.pid; // 子线程列表

    /************更新子线程的info***************/
    p_child->task.tree_info.type = TYPE_THREAD;
    // 亲父进程，创建它的那个线程，注意，这个是创建它的那个线程p_proc_current
    p_child->task.tree_info.real_ppid = p_proc_current->task.pid;
    p_child->task.tree_info.ppid = p_parent->task.pid; // 当前父进程
    p_child->task.tree_info.child_p_num = 0;           // 子进程数量
    // p_child->task.tree_info.child_process[NR_CHILD_MAX] = pid;//子进程列表
    p_child->task.tree_info.child_t_num = 0; // 子线程数量
    // p_child->task.tree_info.child_thread[NR_CHILD_MAX];//子线程列表
    p_child->task.tree_info.text_hold = 0; // 是否拥有代码，子进程不拥有代码
    p_child->task.tree_info.data_hold = 0; // 是否拥有数据，子进程拥有数据

    return 0;
}

/**********************************************************
 *		pthread_stack_init			//add by visual
 *2016.5.26 申请子线程的栈，并重置其esp
 *************************************************************/
static int pthread_stack_init(process_t *p_child, process_t *p_parent, pthread_attr_t *attr) {
    int addr_lin;
    char *p_reg; // point to a register in the new kernel stack, added by xw,
    UNUSED(p_reg);
    UNUSED(addr_lin);

    p_child->task.memmap.stack_lin_limit = (u32)(attr->stackaddr - attr->stacksize); // 子线程的栈界
    p_parent->task.memmap.stack_child_limit += attr->stacksize;                      // 分配16K
    p_child->task.memmap.stack_lin_base = (u32)attr->stackaddr; // 子线程的基址

    kern_mmap(p_child, NULL, p_child->task.memmap.stack_lin_limit, attr->stacksize,
              PROT_READ | PROT_WRITE, MAP_PRIVATE, 0);
    proc_kstacktop(p_child)->esp = p_child->task.memmap.stack_lin_base;
    proc_kstacktop(p_child)->ebp = p_child->task.memmap.stack_lin_base;

    return 0;
}

static int pthread_heap_init(process_t *p_child, process_t *p_parent) {
    //! 子线程使用父进程的堆
    p_child->task.memmap.heap_lin_base = (u32) & (p_parent->task.memmap.heap_lin_base);
    p_child->task.memmap.heap_lin_limit = (u32) & (p_parent->task.memmap.heap_lin_limit);
    return 0;
}

pthread_t kern_pthread_self() {
    return p_proc_current->task.pthread_id;
}

void kern_pthread_exit(void *retval) {
    process_t *p_proc = p_proc_current;

    if (p_proc->task.who_wait_flag != (u32)(-1)) { // 有进程正在等待自己
        process_t *p_father = pid2proc(p_proc->task.who_wait_flag);
        p_proc->task.who_wait_flag = (u32)-1;
        wakeup(p_father);
    }

    p_proc->task.retval = retval;
    p_proc->task.stat = ZOMBY;

    rq_remove(p_proc);
    sched_yield();
}

int kern_pthread_join(pthread_t thread, void **retval) {
    process_t *p_proc_father = p_proc_current; // 发起等待的进程

    if (p_proc_father->task.tree_info.child_t_num == 0) {
        kprintf("no child_pthread!! error\n");
        return -1;
    }

    process_t *p_proc_child = &proc_table[thread]; // 要等待的目标子线程

    // 子线程不是joinable的
    if (p_proc_child->task.attr.detachstate != PTHREAD_CREATE_JOINABLE) {
        kprintf("pthread is not joinable!! error\n");
        return -1;
    }
    while (1) {
        spinlock_lock_or_yield(&p_proc_father->task.lock);
        if (p_proc_child->task.stat != ZOMBY) {
            // 挂起，并告知被等待的线程，线程退出时再唤醒
            spinlock_lock_or_yield(&p_proc_child->task.lock);
            p_proc_child->task.who_wait_flag = p_proc_father->task.pid;
            spinlock_release(&p_proc_child->task.lock);
            spinlock_release(&p_proc_father->task.lock);
            wait_event(p_proc_father);
        } else {
            spinlock_lock_or_yield(&p_proc_child->task.lock);
            // 获取返回值
            if (retval != NULL) { *retval = p_proc_child->task.retval; }

            p_proc_father->task.tree_info.child_t_num--;

            p_proc_father->task.tree_info.child_thread[thread] = 0;

            // 释放栈物理页
            //  free_seg_phypage(p_proc_child->task.pid, MEMMAP_STACK);

            // 释放栈页表
            //  free_seg_pagetbl(p_proc_child->task.pid, MEMMAP_STACK);
            spinlock_release(&p_proc_child->task.lock);
            // 释放PCB
            free_pcb(p_proc_child);

            spinlock_release(&p_proc_father->task.lock);
            break;
        }
    }

    return 0;
}
