#include <minios/ioremap.h>
#include <minios/buddy.h>
#include <minios/memman.h>
#include <minios/assert.h>
#include <minios/page.h>

static void rewrite_kernel_ptes(uintptr_t va, phyaddr_t pa, size_t size, int attr) {
    assert(PGOFF(va) == 0 && PGOFF(pa) == 0);
    uint32_t *pd = K_PHY2LIN(kernel_pde_phy);
    for (size_t off = 0; off < size; off += PGSIZE) {
        uint32_t *pt = K_PHY2LIN(PTE_ADDR(pd[PDX(va + off)]));
        pt[PTX(va + off)] = (pa + off) | attr;
    }
}

void *ioremap(phyaddr_t phy_addr, size_t size) {
    const off_t off = PGOFF(phy_addr);
    const int order = buddy_fit_order(size + off);
    auto page = buddy_alloc(bud, order);
    if (page == NULL) { return NULL; }
    const phyaddr_t pa = pfn_to_phy(page_to_pfn(page));
    if (pa >= SZ_1G) {
        buddy_free(bud, page);
        return NULL;
    }
    void *addr_aligned = K_PHY2LIN(pa);
    rewrite_kernel_ptes(ptr2u(addr_aligned), phy_addr - off, page->size_hint * SZ_4K,
                        PG_P | PG_USS | PG_RWW);
    refresh_page_cache();
    return addr_aligned + off;
}

void iounmap(void *addr) {
    if (addr == NULL) { return; }
    const uintptr_t va = ROUNDDOWN(ptr2u(addr), PGSIZE);
    const phyaddr_t pa = K_LIN2PHY(va);
    assert(va >= KernelLinBase && pa <= SZ_1G);
    assert(PGOFF(pa) == 0);
    auto page = pfn_to_page(phy_to_pfn(pa));
    assert(page->state == PAGESTATE_ALLOCATED);
    rewrite_kernel_ptes(va, pa, page->size_hint * SZ_4K, PG_P | PG_USS | PG_RWW);
    buddy_free(bud, page);
    refresh_page_cache();
}
