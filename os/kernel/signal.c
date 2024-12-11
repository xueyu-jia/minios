#include <errno.h> // IWYU pragma: keep
#include <kernel/interrupt_x86.h>
#include <kernel/ksignal.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <klib/assert.h>
#include <klib/compiler.h>
#include <klib/string.h>

int kern_signal(int sig, void* handler, void* _Handler) {
    if ((u32)sig >= NR_SIGNALS) { return -1; }
    auto proc = proc_real(p_proc_current);
    proc->task.sig_handler[sig] = handler;
    proc->task._Handler = _Handler;
    return 0;
}

int do_signal(int sig, void* handler, void* _Handler) {
    return kern_signal(sig, handler, _Handler);
}

int kern_sigsend(int pid, Sigaction* action) {
    //! TODO: support kill process group
    if (!(pid >= 0 && pid < NR_PCBS)) { return -1; }

    const int sig_bit = 1 << action->sig;
    auto task = &proc_real(&proc_table[pid])->task;
    if (task->sig_handler[action->sig] == SIG_IGN ||
        (task->sig_set & sig_bit)) {
        return -1;
    }

    task->sig_set |= sig_bit;
    task->sig_arg[action->sig] = action->arg;
    return 0;
}

int do_sigsend(int pid, Sigaction* sigaction_p) {
    return kern_sigsend(pid, sigaction_p);
}

void kern_sigreturn(int ebp) {
    auto user_regs = p_proc_current->task.context.esp_save_int;
    //! NOTE: see process_signal(), here old_esp refers to the `saved context
    //! reserved for sigreturn`
    //! ATTENTION: the first plus 4 here refers to the saved ebp pushed in the
    //! _Handler
    const u32 old_esp = ebp + 4 + 4 + sizeof(Sigaction);
    memcpy(user_regs, (void*)old_esp, sizeof(STACK_FRAME));
}

void do_sigreturn(int ebp) {
    kern_sigreturn(ebp);
}

int sys_signal() {
    return do_signal(get_arg(1), (void*)get_arg(2), (void*)get_arg(3));
}

int sys_sigsend() {
    return do_sigsend(get_arg(1), (Sigaction*)get_arg(2));
}

void sys_sigreturn() {
    do_sigreturn(get_arg(1));
}

void process_signal() {
    auto real_proc = proc_real(p_proc_current);
    auto real_task = &real_proc->task;

    if (real_task->sig_set == 0) { return; }

    int sig_nr = -1;
    while (++sig_nr < NR_SIGNALS) {
        const int mask = 1 << sig_nr;
        //! FIXME: some signals cannot be ignored
        const bool ignored = real_task->sig_handler[sig_nr] == SIG_IGN;
        if (real_task->sig_set & mask && !ignored) { break; }
    }
    if (sig_nr == NR_SIGNALS) { return; }

    real_task->sig_set ^= 1 << sig_nr;

    void* handler = real_task->sig_handler[sig_nr];
    bool has_custom_handler = false;
    if (false) {
    } else if (handler == SIG_DFL) {
        //! TODO: more default handler
        switch (sig_nr) {
            case SIGINT: {
                do_exit(sig_nr);
            } break;
            default: {
            } break;
        }
    } else if (handler == SIG_IGN) {
        unreachable();
    } else {
        has_custom_handler = true;
    }

    if (!has_custom_handler) { return; }

    //! NOTE: we do need to force the main thread, i.e. real proc, to handle the
    //! signal, actually, all threads are available to complete the stuff, the
    //! only thing we need to guarantee is that the signal is handled at most
    //! once
    //! TODO: the given signal can be blocked by the current thread, in this
    //! case, we must select another thread to handle the signal
    auto sigproc_proc = p_proc_current;

    //! ATTENTION: info of signal action is saved in the main thread, i.e. real
    //! proc, get it from there and copy it to the signal processing thread

    Sigaction sigaction = {
        .sig = sig_nr,
        .handler = real_proc->task.sig_handler[sig_nr],
        .arg = real_proc->task.sig_arg[sig_nr],
    };

    disable_int_begin();

    auto sigproc_user_regs = sigproc_proc->task.context.esp_save_int;

    //! NOTE: control flow: kernel -> _Handler (noreturn) -> sigreturn -> kernel

    u32 frame_ptr = sigproc_user_regs->esp;

    //! NOTE: saved context reserved for sigreturn
    frame_ptr -= sizeof(STACK_FRAME);
    memcpy((void*)frame_ptr, sigproc_user_regs, sizeof(STACK_FRAME));

    //! NOTE: args passed to _Handler
    //! FIXME: args copy method below may violate the arch abi rules
    frame_ptr -= sizeof(Sigaction);
    memcpy((void*)frame_ptr, &sigaction, sizeof(Sigaction));

    //! NOTE: stack place of return address, is ignored here since our wrapped
    //! handler is a noreturn method, it will trap into another syscall at the
    //! end and directly turn to sigreturn
    frame_ptr -= sizeof(void*);

    sigproc_user_regs->esp = frame_ptr;
    sigproc_user_regs->eip = (u32)real_proc->task._Handler;

    disable_int_end();
}
