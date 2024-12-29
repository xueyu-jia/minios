#include <paging.h>
#include <string.h>
#include <x86.h>
#include <regs.h>
#include <memory.h>
#include <layout.h>
#include <terminal.h>

static phyaddr_t alloc_pte() {
    static phyaddr_t NEXT_PTE_PTR = PageTblBase;
    const phyaddr_t pte_phy_addr = NEXT_PTE_PTR;
    NEXT_PTE_PTR += PGSIZE;
    memset(u2ptr(pte_phy_addr), 0, PGSIZE);
    return pte_phy_addr;
}

phyaddr_t phy_kmalloc(size_t size) {
    static phyaddr_t NEXT_PHY_ADDR = PhyMallocBase;
    const phyaddr_t phy_addr = NEXT_PHY_ADDR;
    NEXT_PHY_ADDR += size;
    return phy_addr;
}

void map_laddr(phyaddr_t cr3, uintptr_t laddr, phyaddr_t paddr, u32 pte_attr) {
    uintptr_t *const pde_ptr = u2ptr(cr3);
    if ((pde_ptr[PDX(laddr)] & PG_P) == 0) {
        const phyaddr_t pte_phy = alloc_pte();
        pde_ptr[PDX(laddr)] = pte_phy | PG_P | PG_USU | PG_RWW;
    }
    uintptr_t *const pte_ptr = u2ptr(PTE_ADDR(pde_ptr[PDX(laddr)]));
    if ((pte_ptr[PTX(laddr)] & PG_P) != 0) { return; }
    pte_ptr[PTX(laddr)] = paddr | pte_attr;
}

void setup_paging() {
    lprintf("info: setup paging\n");
    lprintf("info: page directory base: 0x%p\n", PageDirBase);
    const phyaddr_t cr3 = PageDirBase;
    memset(u2ptr(cr3), 0, PGSIZE);

    const auto fmi = __fmi_ptr();
    for (size_t i = 0; i < fmi->count; ++i) {
        const auto range = fmi->page_ranges[i];
        //! TODO: i386 only
        if (range.base >= NUM_1G) { continue; }
        const phyaddr_t limit = MIN(range.limit, NUM_1G);
        lprintf("mapping va 0x%p~0x%p to pa 0x%p~0x%p\n", range.base, limit - 1, range.base,
                limit - 1);
        lprintf("mapping va 0x%p~0x%p to pa 0x%p~0x%p\n", range.base, limit - 1,
                K_PHY2LIN(range.base), K_PHY2LIN(limit - 1));
        for (phyaddr_t addr = range.base; addr < limit; addr += PGSIZE) {
            map_laddr(cr3, addr, addr, PG_P | PG_USU | PG_RWW);
            map_laddr(cr3, (uintptr_t)K_PHY2LIN(addr), addr, PG_P | PG_USU | PG_RWW);
        }
    }

    const phyaddr_t pt_limit = alloc_pte();
    lprintf("info: pa 0x%p~0x%p is assigned to page tables, %d in total\n", PageTblBase,
            pt_limit - 1, (pt_limit - PageTblBase) / PGSIZE);

    lcr3(cr3);
    lcr0(rcr0() | CR0_PG);

    lprintf("info: paging setup done\n");
}
