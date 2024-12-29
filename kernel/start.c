#include <minios/protect.h>
#include <minios/proc.h>
#include <minios/layout.h>
#include <minios/kstate.h>
#include <minios/proto.h>
#include <minios/layout.h>
#include <minios/vga.h>
#include <minios/page.h>
#include <minios/assert.h>
#include <minios/memman.h>
#include <klib/stddef.h>
#include <string.h>

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
    phyaddr_t k_pd_base = phy_kmalloc_4k();
    kprintf("info: alloc page directory for kernel at pa 0x%p\n", k_pd_base);
    memset((void *)K_PHY2LIN(k_pd_base), 0, PGSIZE);

    kprintf("info: prealloc page tables for va 0x%p~0x%p\n", ROUNDDOWN(KernelLinMapBase, SZ_4M),
            ROUNDUP(KernelLinMapLimit, SZ_4M) - 1);
    for (uintptr_t va = KernelLinMapBase; va < KernelLinMapLimit; va += SZ_4M) {
        const phyaddr_t pt_base = phy_kmalloc_4k();
        assert(PGOFF(pt_base) == 0 && "invalid physical page address");
        memset(K_PHY2LIN(pt_base), 0, PGSIZE);
        write_page_pde(k_pd_base, va, pt_base, PG_P | PG_USS | PG_RWW);
    }

    kprintf("info: mapping va 0x%p~0x%p to pa 0x%p~0x%p for kernel\n", KernelLinBase,
            ROUNDUP(KernelLinBase + kernel_size, PGSIZE) - 1, K_LIN2PHY(KernelLinBase),
            K_LIN2PHY(ROUNDUP(KernelLinBase + kernel_size, PGSIZE) - 1));
    for (uintptr_t va = KernelLinBase; va < KernelLinBase + kernel_size; va += PGSIZE) {
        const int status = lin_mapping_phy_nopid(va, K_LIN2PHY(va), k_pd_base,
                                                 PG_P | PG_USS | PG_RWW, PG_P | PG_USS | PG_RWW);
        assert(status == 0 && "failed to map kernel page");
    }

    lcr3(k_pd_base);

    kernel_pde_phy = k_pd_base;
}

#include <minios/uart.h>

void cstart() {
    kstate_on_init = true;

    init_simple_serial();

    memory_init();

    init_kernel_page();

    init_gdt();
    init_interrupt_controller();
}
