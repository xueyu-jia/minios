#include <minios/protect.h>
#include <minios/proc.h>
#include <minios/layout.h>
#include <minios/kstate.h>
#include <minios/layout.h>
#include <minios/vga.h>
#include <minios/page.h>
#include <minios/assert.h>
#include <minios/memman.h>
#include <minios/console.h>
#include <minios/interrupt.h>
#include <minios/asm.h>
#include <klib/stddef.h>
#include <multiboot.h>
#include <string.h>
#include <ctype.h>

extern void refresh_gdt();

static void init_gdt() {
    init_descriptor(&gdt[INDEX_DUMMY], 0, 0, 0);
    init_descriptor(&gdt[INDEX_FLAT_C], 0, 0xfffff, DA_CR | DA_32 | DA_LIMIT_4K);
    init_descriptor(&gdt[INDEX_FLAT_RW], 0, 0xfffff, DA_DRWA | DA_32 | DA_LIMIT_4K);

    init_descriptor(&gdt[INDEX_VIDEO], ptr2u(K_PHY2LIN(V_MEM_BASE)), 0xffff, DA_DRW | DA_DPL3);

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    tss.iobase = sizeof(tss);
    init_descriptor(&gdt[INDEX_TSS], vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss), sizeof(tss) - 1,
                    DA_386TSS);

    for (int i = 0; i < NR_PCBS; ++i) {
        init_descriptor(&gdt[INDEX_LDT_FIRST + i],
                        vir2phys(seg2phys(SELECTOR_KERNEL_DS), proc_table[i].task.context.ldts),
                        LDT_SIZE * sizeof(descriptor_t) - 1, DA_LDT);
    }

    gdt_ptr.base = ptr2u(gdt);
    gdt_ptr.limit = sizeof(gdt) - 1;

    refresh_gdt();
}

static void init_kernel_page() {
    phyaddr_t k_pd_base = phy_kmalloc(SZ_4K);
    kprintf("info: alloc page directory for kernel at pa 0x%p\n", k_pd_base);
    memset((void*)K_PHY2LIN(k_pd_base), 0, PGSIZE);

    {
        const uintptr_t base = ROUNDDOWN(KernelLinMapBase, SZ_4M);
        const uintptr_t limit = ROUNDUP(KernelLinMapLimit, SZ_4M);
        kprintf("info: prealloc page tables for mmap pages at va 0x%p~0x%p\n", base, limit - 1);
        for (uintptr_t va = base; va < limit; va += SZ_4M) {
            phyaddr_t pt_base = PHY_NULL;
            if (pte_exist(k_pd_base, va)) {
                pt_base = get_pte_phy_addr(k_pd_base, va);
            } else {
                pt_base = phy_kmalloc(SZ_4K);
                assert(pt_base != PHY_NULL && PGOFF(pt_base) == 0);
            }
            memset(K_PHY2LIN(pt_base), 0, PGSIZE);
            write_page_pde(k_pd_base, va, pt_base, PG_P | PG_USS | PG_RWW);
        }
    }
    {
        const uintptr_t kernel_space_limit =
            ptr2u(K_PHY2LIN(MIN(pfn_to_phy(nr_mmap_pages), K_LIN2PHY(KernelLinLimitMAX))));
        const uintptr_t base = KernelLinBase;
        const uintptr_t limit = ROUNDUP(kernel_space_limit, PGSIZE);
        kprintf("info: mapping va 0x%p~0x%p to pa 0x%p~0x%p for kernel\n", base, limit - 1,
                K_LIN2PHY(base), K_LIN2PHY(limit - 1));
        for (uintptr_t va = base; va < limit; va += PGSIZE) {
            const int status = lin_mapping_phy_nopid(
                va, K_LIN2PHY(va), k_pd_base, PG_P | PG_USS | PG_RWW, PG_P | PG_USS | PG_RWW);
            assert(status == 0 && "failed to map kernel page");
        }
    }

    lcr3(k_pd_base);

    kernel_pde_phy = k_pd_base;
}

static char* early_klog_handler(char* buf, void* user, int len) {
    UNUSED(user);
    for (int i = 0; i < len; ++i) {
        const char ch = buf[i];
        const bool accepted = isprint(ch) || ch == '\n' || ch == '\t';
        if (accepted) {
            while (!(inb(0x3f8 + 5) & 0x20)) {}
            outb(0x3f8, ch);
        }
    }
    return buf;
}

static void early_init_klog() {
    outb(0x3f8 + 1, 0x00); //<! disable all interrupts
    outb(0x3f8 + 3, 0x80); //<! enable dlab (set baud rate divisor)
    outb(0x3f8 + 0, 0x03); //<! set divisor to 3 (lo byte) 38400 baud
    outb(0x3f8 + 1, 0x00); //<! (hi byte)
    outb(0x3f8 + 3, 0x03); //<! 8 bits, no parity, one stop bit
    outb(0x3f8 + 2, 0xc7); //<! enable fifo, clear them, with 14-byte threshold
    outb(0x3f8 + 4, 0x0b); //<! irqs enabled, rts/dsr set
    outb(0x3f8 + 4, 0x1e); //<! set in loopback mode, test the serial chip

    //! ATTENTION: it is a double insurance against external hardware and software status anomalies,
    //! here, before entering loopback mode, all data has been received and no more data is about to
    //! be received, but the fact is that there are always some abnormal circumstances that will
    //! make the UART device receive some unread data from unknown sources before switching the mode
    while (inb(0x3f8 + 5) & 0x20) { (void)inb(0x3f8); }

    //! test the serial chip by sending a byte and checking if it is received correctly
    const uint8_t test_byte = 0xae;
    while (!(inb(0x3f8 + 5) & 0x20)) {}
    outb(0x3f8, test_byte);
    while (!(inb(0x3f8 + 5) & 0x01)) {}
    const uint8_t received_byte = inb(0x3f8);
    assert(received_byte == test_byte);

    //! if serial is not faulty set it in normal operation mode, i.e. not-loopback with IRQs enabled
    //! and OUT#1 and OUT#2 bits enabled
    outb(0x3f8 + 4, 0x0f);

    extern void* __klog_handler_cb;
    __klog_handler_cb = early_klog_handler;
}

void cstart(phyaddr_t mbi_phy_addr) {
    kstate_on_init = true;

    early_init_klog();

    //! FIXME: kernel deps on the page table setup in our bootloader
    kstate_mbi = K_PHY2LIN(mbi_phy_addr);

    memory_init();

    init_kernel_page();

    init_gdt();
    init_interrupt_controller();
}
