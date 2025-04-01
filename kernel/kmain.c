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
#include <minios/time.h>
#include <minios/protect.h>
#include <minios/sched.h>
#include <minios/shm.h>
#include <minios/vfs.h>
#include <driver/acpi/acpi.h>
#include <driver/pci/pci.h>
#include <driver/pci/vendor.h>
#include <driver/ata/ahci.h>
#include <driver/rtc/rtc.h>
#include <driver/serial/uart.h>
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
    kprintf("%02x:%02x:%02x vendor_id=%#x \"%s\" device_id=%#x \"%s\"\n", PCI_GET_BUS(info->id),
            PCI_GET_DEV(info->id), PCI_GET_FUNC(info->id), info->vendor_id, vendor_info.vendor_name,
            info->device_id, vendor_info.device_name);
    return true;
}

static void scan_pci_tree() {
    pci_visit(do_scan_pci_tree, NULL);
}

static void write_kernel_pte(uintptr_t va, phyaddr_t pa, int attr) {
    assert(PGOFF(va) == 0 && PGOFF(pa) == 0);
    uint32_t *pd = K_PHY2LIN(kernel_pde_phy);
    uint32_t *pt = K_PHY2LIN(PTE_ADDR(pd[PDX(va)]));
    pt[PTX(va)] = (pa) | attr;
}

static void switch_kernel_stack() {
    const int nr_pages = SZ_1M / PGSIZE;
    auto page = buddy_alloc_kernel_pages(buddy_fit_order(SZ_1M));
    assert(page != NULL);

    auto lower_guard_page = &page[0];
    write_kernel_pte(ptr2u(kpage_lin(lower_guard_page)), 0, 0);
    auto upper_guard_page = &page[nr_pages - 1];
    write_kernel_pte(ptr2u(kpage_lin(upper_guard_page)), 0, 0);

    for (int i = 0; i < nr_pages; ++i) { page[i].state = PAGESTATE_CRITICAL; }
    lower_guard_page->state = PAGESTATE_INVAL;
    upper_guard_page->state = PAGESTATE_INVAL;
    bud->total_mem -= SZ_1M;
    bud->used_mem -= SZ_1M;

    memset(kpage_lin(&lower_guard_page[1]), 0, (nr_pages - 2) * PGSIZE);
    const void *stack_top = kpage_lin(upper_guard_page);
    asm volatile(
        "leave\n"
        "movl (%%esp), %%ebx\n"
        "movl %%eax, %%esp\n"
        "jmp *%%ebx"
        :
        : "a"(stack_top));
}

static void report_available_memory() {
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

int kernel_main() {
    might_setup_debugging_environment();

    //! ATTENTION: ints is disabled through the whole `kernel_main` except for init_fs

    switch_kernel_stack();

    disp_pos = 0;
    kstate_reenter_cntr = 0;

    kprintf("info: enter kernel\n");

    sched_init();
    const bool proc_inited = init_proc() && init_cpu();
    assert(proc_inited && "failed to init proc and cpu");

    kprintf("info: init acpi\n");
    init_acpi();
    kprintf("info: scan acpi device tree\n");
    scan_acpi_sdst();

    kprintf("info: scan pci tree\n");
    scan_pci_tree();

    kprintf("info: init devices\n");
    init_devices();
    rtc_probe();
    serial_uart_probe();
    init_kb();
    ahci_sata_init();
    init_hd();

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
        //! ATTENTION: order matters, register must before hd_open_and_init and init_fs
        register_fs_types();
        init_buffer(64);
        hd_open_and_init();
        const int boot_dev_idx =
            (kstate_mbi->flags & BIT(1)) ? (kstate_mbi->boot_device >> 24) & 0x7f : 0;
        init_fs(HD_DEV_MAKE_BLK(DEV_BLK_HD_SATA, boot_dev_idx));
    }
    disable_int();

    kprintf("info: init ttys\n");
    init_ttys();
    spinlock_init(&video_mem_lock, "vmem");

    report_available_memory();
    kern_get_time(&kstate_os_startup_time);

    kprintf("info: kernel init done\n");
    kstate_on_init = false;

    p_proc_current = &proc_table[PID_INIT];
    p_proc_current->task.cwd = vfs_root;
    cr3_ready = p_proc_current->task.cr3;

    restart_initial();

    unreachable();
}
