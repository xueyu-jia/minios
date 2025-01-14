#pragma once

#include <klib/stddef.h>
#include <klib/stdint.h>
#include <klib/size.h>
#include <klib/sys/types.h>
#include <stdbool.h>

#define PGSHIFT 12
#define PGSIZE (1 << PGSHIFT)

#define PDXSHIFT 22
#define PTXSHIFT 12

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

#define phy_to_pfn(x) ((x) >> PGSHIFT)
#define pfn_to_phy(x) ((x) << PGSHIFT)

extern u32 kernel_pde_phy;

void refresh_page_cache();

int init_proc_page(u32 pid);
void write_page_pde(u32 PageDirPhyAddr, u32 va, u32 TblPhyAddr, u32 Attribute);
int kmapping_phy(u32 phy_addr);
int kunmapping_phy(u32 phy_addr);
int map_laddr(u32 va, u32 phy_addr, u32 pid, u32 pde_attr, u32 pte_attr);
int lin_mapping_phy_nopid(u32 va, u32 pa, u32 kernel_pde_addr_phy, u32 pde_attr, u32 pte_attr);

void free_all_pagetbl(u32 pid);
void free_pagetbl(u32 pid, u32 va);
void free_pagedir(u32 pid);
u32 get_page_phy_addr(u32 pid, u32 va);
void clear_pte(u32 pid, u32 va);
void update_heap_limit(u32 pid, int tag);
u32 get_heap_limit(u32 pid);

u32 get_pde_phy_addr(u32 pid);
u32 get_pte_phy_addr(u32 pd_base, u32 va);
u32 get_page_phy_addr_nopid(u32 pt_base, u32 va);
bool pte_exist(u32 pd_base, u32 va);
bool phy_exist(u32 pt_base, u32 va);

static inline u32 get_pde_index(u32 va) {
    return va >> PDXSHIFT;
}

static inline u32 get_pte_index(u32 va) {
    return (((va) & 0x003fffff) >> PTXSHIFT);
}
