#include <minios/pthread.h>
#include <minios/memman.h>
#include <minios/mmap.h>
#include <minios/proc.h>
#include <minios/console.h>
#include <minios/interrupt.h>
#include <minios/sched.h>
#include <minios/assert.h>
#include <fs/fs.h>
#include <klib/size.h>
#include <errno.h>
#include <string.h>

static int pthread_copy_pcb(process_t *ch, process_t *fa) {
    const int ch_pid = ch->task.pid;

    auto ch_task = fa->task;
    ch_task.stat = SLEEPING;
    spinlock_release(&ch_task.lock);

    //! NOTE: signal parts are compelety shared with the main thread, i.e. real
    //! proc, we do not need to maintain them in child's pcb, however, handlers
    //! can be kept for convenient use
    ch_task.sig_set = 0;

    disable_int_begin();
    ch->task = ch_task;
    disable_int_end();

    // 线程使用父进程的mmap 链表，此两项直接重置为空
    init_mem_page(&ch->task.memmap.anon_pages, MEMPAGE_AUTO);
    list_init(&ch->task.memmap.vma_map);
    memcpy((char *)proc_kstacktop(ch), proc_kstacktop(fa), P_STACKTOP);

    ch->task.pid = ch_pid;
    init_user_cpu_context(&ch->task.context, ch->task.pid);

    return 0;
}

static void pthread_tree_info_insert_thread_impl(process_t *proc, pthread_t th_pid) {
    auto info = &proc->task.tree_info;
    assert(info->child_t_num < NR_CHILD_MAX);
    const int index = info->child_t_num++;
    if (info->child_thread[index] == 0) {
        info->child_thread[index] = th_pid;
        return;
    }
    for (int i = 0; i < NR_CHILD_MAX; ++i) {
        if (info->child_thread[i] == 0) {
            info->child_thread[i] = th_pid;
            return;
        }
    }
    unreachable();
}

static void pthread_tree_info_insert_thread(pthread_t th_pid) {
    auto th_fa = p_proc_current;
    auto fa = proc_real(p_proc_current);
    if (fa != th_fa) { pthread_tree_info_insert_thread_impl(fa, th_pid); }
    pthread_tree_info_insert_thread_impl(th_fa, th_pid);
}

static void pthread_tree_info_remove_thread(pthread_t tid) {
    auto th_fa = p_proc_current;
    auto fa = proc_real(p_proc_current);
    assert(tid >= 1 && tid <= NR_CHILD_MAX);
    if (fa != th_fa) {
        --fa->task.tree_info.child_t_num;
        fa->task.tree_info.child_thread[tid - 1] = 0;
    }
    --th_fa->task.tree_info.child_t_num;
    th_fa->task.tree_info.child_thread[tid - 1] = 0;
}

static void pthread_update_tree_info(process_t *ch) {
    //! NOTE: about th tree info: proc owns all its descendants and each thread marks its all direct
    //! descendants
    pthread_tree_info_insert_thread(ch->task.pid);

    ch->task.tree_info.type = TYPE_THREAD;
    ch->task.tree_info.real_ppid = p_proc_current->task.pid;
    ch->task.tree_info.ppid = proc_real(p_proc_current)->task.pid;
    ch->task.tree_info.child_p_num = 0;
    ch->task.tree_info.child_t_num = 0;
    ch->task.tree_info.text_hold = false;
    ch->task.tree_info.data_hold = false;
}

static void pthread_stack_init(process_t *ch, process_t *fa, pthread_attr_t *attr) {
    assert(ch != NULL);
    assert(fa != NULL);
    assert(attr != NULL);

    ch->task.memmap.stack_lin_limit = ptr2u(attr->stackaddr) - attr->stacksize;
    fa->task.memmap.stack_child_limit += attr->stacksize;
    ch->task.memmap.stack_lin_base = ptr2u(attr->stackaddr);

    kern_mmap(ch, NULL, ch->task.memmap.stack_lin_limit, attr->stacksize, PROT_READ | PROT_WRITE,
              MAP_PRIVATE, 0);

    auto frame = proc_kstacktop(ch);
    frame->esp = ch->task.memmap.stack_lin_base;
    frame->ebp = ch->task.memmap.stack_lin_base;
}

static void pthread_heap_init(process_t *p_child, process_t *p_parent) {
    p_child->task.memmap.heap_lin_base_ref = &p_parent->task.memmap.heap_lin_base;
    p_child->task.memmap.heap_lin_limit_ref = &p_parent->task.memmap.heap_lin_limit;
}

static process_t *lookup_thread(pthread_t tid) {
    process_t *fa = proc_real(p_proc_current);
    //! ATTENTION: tid - 1 equals to thread index in proc, maintain it carefully
    const auto tree = &fa->task.tree_info;
    if (!(tid >= 1 && tid <= NR_CHILD_MAX)) { return NULL; }
    const pid_t th_pid = tree->child_thread[tid - 1];
    if (th_pid == 0) { return NULL; }
    return pid2proc(th_pid);
}

int kern_pthread_create_internal(pthread_t *tid, const pthread_attr_t *attr,
                                 pthread_entry_t wrapped_start_routine, void *arg) {
    process_t *fa = proc_real(p_proc_current);

    pthread_attr_t th_attr = {};
    if (attr == NULL) {
        th_attr.detachstate = PTHREAD_CREATE_JOINABLE;
        th_attr.schedpolicy = SCHED_FIFO;
        th_attr.inheritsched = PTHREAD_INHERIT_SCHED;
        th_attr.scope = PTHREAD_SCOPE_PROCESS;
        th_attr.guardsize = 0;
        th_attr.stackaddr_set = 0;
        th_attr.stacksize = SZ_16K;
        th_attr.stackaddr = u2ptr(fa->task.memmap.stack_child_limit + th_attr.stacksize);
    } else {
        th_attr = *attr;
    }

    process_t *ch = alloc_pcb();
    if (ch == NULL) { return -EAGAIN; }

    spinlock_lock_or_yield(&fa->task.lock);
    pthread_copy_pcb(ch, fa);

    //! NOTE: order matters, ensures first copy then acquire the lock
    spinlock_acquire(&ch->task.lock);

    pthread_update_tree_info(ch);
    pthread_stack_init(ch, fa, &th_attr);
    pthread_heap_init(ch, fa);

    //! TODO: ensures the thread terminates with exit or pthread_exit
    stack_frame_t *frame = proc_kstacktop(ch);
    frame->eip = ptr2u(wrapped_start_routine);
    frame->esp = ch->task.memmap.stack_lin_base;
    frame->eax = 0;

    ch->task.attr = th_attr;
    ch->task.who_wait_flag = -1;

    strcpy(ch->task.p_name, "pthread");

    u32 *stack = u2ptr(frame->esp);
    frame->esp -= SZ_4 * 2;
    stack[-1] = ptr2u(arg);
    stack[-2] = 0xbaadf00d; //<! unreachable retaddr, place it for debug

    //! ATTENTION: maintain it carefully
    ch->task.tid = fa->task.tree_info.child_t_num;

    ch->task.stat = READY;
    rq_insert(ch);

    spinlock_release(&ch->task.lock);
    spinlock_release(&fa->task.lock);

    if (tid != NULL) { *tid = ch->task.tid; }

    return 0;
}

pthread_t kern_pthread_self() {
    return p_proc_current->task.tid;
}

void kern_pthread_exit(void *retval) {
    if (p_proc_current->task.who_wait_flag != (u32)(-1)) { // 有进程正在等待自己
        process_t *fa = pid2proc(p_proc_current->task.who_wait_flag);
        p_proc_current->task.who_wait_flag = (u32)-1;
        wakeup(fa);
    }

    p_proc_current->task.retval = retval;
    p_proc_current->task.stat = ZOMBY;

    rq_remove(p_proc_current);
    sched_yield();
}

int kern_pthread_join(pthread_t thread, void **retval) {
    process_t *fa = p_proc_current;

    process_t *ch = lookup_thread(thread);
    if (ch == NULL) { return -ESRCH; }
    if (ch->task.attr.detachstate != PTHREAD_CREATE_JOINABLE) { return -EINVAL; }

    while (true) {
        spinlock_lock_or_yield(&fa->task.lock);
        if (ch->task.stat == ZOMBY) {
            spinlock_lock_or_yield(&ch->task.lock);
            if (retval != NULL) { *retval = ch->task.retval; }
            pthread_tree_info_remove_thread(thread);
            spinlock_release(&ch->task.lock);
            free_pcb(ch);
            spinlock_release(&fa->task.lock);
            break;
        }
        // 挂起，并告知被等待的线程，线程退出时再唤醒
        spinlock_lock_or_yield(&ch->task.lock);
        ch->task.who_wait_flag = fa->task.pid;
        spinlock_release(&ch->task.lock);
        spinlock_release(&fa->task.lock);
        wait_event(fa);
    }

    return 0;
}
