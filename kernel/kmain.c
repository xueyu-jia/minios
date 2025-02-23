#include <minios/ahci.h>
#include <minios/asm.h>
#include <minios/assert.h>
#include <minios/blame.h>
#include <minios/buddy.h>
#include <minios/clock.h>
#include <minios/console.h>
#include <minios/dev.h>
#include <minios/hd.h>
#include <minios/keyboard.h>
#include <minios/interrupt.h>
#include <minios/kstate.h>
#include <minios/layout.h>
#include <minios/mmap.h>
#include <minios/msg.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/protect.h>
#include <minios/sched.h>
#include <minios/shm.h>
#include <minios/vfs.h>
#include <driver/pci/pci.h>
#include <driver/pci/vendor.h>
#include <fs/devfs/devfs.h>

#ifdef GDBSTUB
#ifdef OPT_DISP_SERIAL
#error "gdb stub is not compatible with serial display"
#endif
#include "gdbstub-x86.h" // IWYU pragma: export
#define might_setup_debugging_environment() gdb_sys_init()
#else
#define might_setup_debugging_environment()
#endif

static bool init_proc() {
    for (int i = 0; i < NR_TASKS; ++i) {
        int pid = kthread_create(task_table[i].name, (void *)task_table[i].initial_eip,
                                 task_table[i].rt, task_table[i].rpl, task_table[i].priority_nice);
        disable_int_begin();
        rq_insert(&proc_table[pid]);
        disable_int_end();
    }

    extern void initial();
    int initial_pid = kthread_create("initial", initial, false, RPL_TASK, 0);
    assert(initial_pid == PID_INIT && "unexpected pid for initial process");
    disable_int_begin();
    rq_insert(&proc_table[initial_pid]);
    disable_int_end();

    process_t *init = &proc_table[PID_INIT];
    init->task.tree_info.type = TYPE_PROCESS;         // 当前是进程还是线程
    init->task.tree_info.real_ppid = -1;              // 亲父进程，创建它的那个进程
    init->task.tree_info.ppid = -1;                   // 当前父进程
    init->task.tree_info.child_p_num = 0;             // 子进程数量
    init->task.tree_info.child_t_num = 0;             // 子线程数量
    init->task.memmap.vpage_lin_base = VpageLinBase;  // 保留内存基址
    init->task.memmap.vpage_lin_limit = VpageLinBase; // 保留内存界限

    return true;
}

static bool init_cpu() {
    //! TODO: not implemented yet
    return true;
}

static bool do_scan_pci_tree(pci_device_info_t *info, void *arg) {
    UNUSED(arg);
    pci_vendor_info_t vendor_info = {};
    const bool found = pci_lookup_vendor(info->vendor_id, info->device_id, &vendor_info);
    if (!found) {
        vendor_info.vendor_name = "Unknown";
        vendor_info.device_name = "Unknown";
    }
    kprintf("visit pci device: vendor_id=%#x \"%s\" device_id=%#x \"%s\"\n", info->vendor_id,
            vendor_info.vendor_name, info->device_id, vendor_info.device_name);
    return true;
}

static void scan_pci_tree() {
    pci_visit(do_scan_pci_tree, NULL);
}

int kernel_main() {
    might_setup_debugging_environment();

    //! ATTENTION: ints is disabled through the whole `kernel_main` except for init_fs

    disp_pos = 0;
    kstate_reenter_cntr = 0;

    kprintf("info: enter kernel\n");

    sched_init();
    const bool proc_inited = init_proc() && init_cpu();
    assert(proc_inited && "failed to init proc and cpu");

    kprintf("info: scan pci tree\n");
    scan_pci_tree();

    kprintf("info: init devices\n");
    init_kb();
    ahci_sata_init();
    init_hd();
    init_devices();

    kprintf("info: init ipc objects\n");
    init_msgq();
    init_shm();

    //! ATTENTION: order matters
    kprintf("info: init jiffies clock\n");
    init_clock();
    early_clock_sync();
    kprintf("info: init tsc ns freq: %lf\n", tsc_ns_freq);

    kprintf("info: init fs\n");
    enable_int();
    {
        //! ATTENTION: order matters, register must before init_open_hd and init_fs
        register_fs_types();
        init_buffer(64);
        init_open_hd();
        init_fs(SATA_BASE);
    }
    disable_int();

    kprintf("info: init ttys\n");
    init_ttys();
    spinlock_init(&video_mem_lock, "vmem");

    {
        const size_t msize = kern_total_mem_size();
        struct {
            const char *name;
            size_t unit;
        } units[] = {
            {"GB", SZ_1G},
            {"MB", SZ_1M},
            {"KB", SZ_1K},
            {"B", SZ_1},
        };
        for (int i = 0; i < ARRAY_SIZE(units); ++i) {
            if (msize < units[i].unit) { continue; }
            kprintf("info: memory available for kernel: %.2f%s\n", msize * 1. / units[i].unit,
                    units[i].name);
            break;
        }
    }

#ifdef BLAME_STAT
    stat_init(); // stat performance
#endif

    kprintf("info: kernel init done\n");
    kstate_on_init = false;

    p_proc_current = &proc_table[PID_INIT];
    p_proc_current->task.cwd = vfs_root;
    cr3_ready = p_proc_current->task.cr3;

    restart_initial();

    unreachable();
}
