#pragma once

#include <minios/const.h>
#include <minios/type.h>
#include <minios/proc.h>
#include <minios/assert.h>
#include <klib/size.h>
#include <stdbool.h>

#define PGSIZE SZ_4K

#define PTSIZE (PGSIZE * NPTENTRIES)
#define PTSHIFT 22

#define PTXSHIFT 12
#define PDXSHIFT 22

#define PGNUM(la) (((uintptr_t)(la)) >> PTXSHIFT)
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0x3ff)
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x3ff)
#define PGOFF(la) (((uintptr_t)(la)) & 0xfff)
#define PGADDR(d, t, o) ((void *)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define PTE_ADDR(pte) ((phyaddr_t)(pte) & ~0xfff)
#define PTE_FLAG(pte) ((size_t)(pte) & 0xfff)

#define PG_P 1
#define PG_RWR 0
#define PG_RWW 2
#define PG_USS 0
#define PG_USU 4
#define PG_PS 64

#define PTE_P 0x001
#define PTE_W 0x002
#define PTE_U 0x004
#define PTE_PWT 0x008
#define PTE_PCD 0x010
#define PTE_A 0x020
#define PTE_D 0x040
#define PTE_PS 0x080
#define PTE_G 0x100

extern u32 kernel_pde_phy;

int init_proc_page(u32 pid);
void write_page_pde(u32 PageDirPhyAddr, u32 AddrLin, u32 TblPhyAddr, u32 Attribute);
int kmapping_phy(u32 phy_addr);
int kunmapping_phy(u32 phy_addr);
int map_laddr(u32 AddrLin, u32 phy_addr, u32 pid,

              u32 pde_Attribute, u32 pte_Attribute);
int lin_mapping_phy_nopid(u32 AddrLin, u32 phy_addr, u32 kernel_pde_addr_phy, u32 pde_Attribute,
                          u32 pte_Attribute);

void free_all_pagetbl(u32 pid);
void free_pagetbl(u32 pid, u32 AddrLin);
void free_pagedir(u32 pid);
u32 get_page_phy_addr(u32 pid, u32 AddrLin);
void clear_kernel_pagepte_low();
void clear_pte(u32 pid, u32 AddrLin);
void update_heap_limit(u32 pid, int tag);
u32 get_heap_limit(u32 pid);

static inline u32 get_pde_index(u32 va) {
    return va >> 22;
}

static inline u32 get_pte_index(u32 va) {
    return (((va) & 0x003fffff) >> 12);
}

static inline u32 get_pde_phy_addr(u32 pid) {
    if (proc_table[pid].task.cr3 == 0) {
        return (u32)NULL;
    } else {
        return ((proc_table[pid].task.cr3) & 0xfffff000);
    }
}

static inline u32 get_pte_phy_addr(u32 pd_base, u32 va) {
    assert(pd_base != 0 && "invalid page directory address");
    return (*((u32 *)K_PHY2LIN(pd_base) + get_pde_index(va))) & 0xfffff000;
}

static inline u32 get_page_phy_addr_nopid(u32 pt_base, u32 va) {
    assert(pt_base != 0 && "invalid page table address");
    return (*((u32 *)K_PHY2LIN(pt_base) + get_pte_index(va))) & 0xfffff000;
}

static inline bool pte_exist(u32 pd_base, u32 va) {
    return ((*((u32 *)K_PHY2LIN(pd_base) + get_pde_index(va))) & PG_P) == PG_P;
}

static inline bool phy_exist(u32 pt_base, u32 va) {
    return ((*((u32 *)K_PHY2LIN(pt_base) + get_pte_index(va))) & PG_P) == PG_P;
}
