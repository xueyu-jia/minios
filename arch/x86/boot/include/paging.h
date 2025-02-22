#pragma once

#include <type.h>

#define PhyMallocBase SZ_1M
#define PageDirBase (SZ_2M - SZ_4K)
#define PageTblBase SZ_2M
#define PageTblLimit (SZ_2M + SZ_4M)

#define PG_P 1   // 页存在属性位
#define PG_RWR 0 // R/W 属性位值, 读/执行
#define PG_RWW 2 // R/W 属性位值, 读/写/执行
#define PG_USS 0 // U/S 属性位值, 系统级
#define PG_USU 4 // U/S 属性位值, 用户级
#define PG_PS 64 // PS属性位值，4K页

#define PGSIZE 0x1000

// A linear address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \---------- PGNUM(la) ----------/
//
// The PDX, PTX, PGOFF, and PGNUM macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// page number field of address
#define PGNUM(la) (((uintptr_t)(la)) >> PTXSHIFT)

// page directory index
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x3FF)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o) ((void*)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define PTSIZE (PGSIZE * NPTENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT 22                   // log2(PTSIZE)

#define PTXSHIFT 12 // offset of PTX in a linear address
#define PDXSHIFT 22 // offset of PDX in a linear address

// Page table/directory entry flags.
#define PTE_P 0x001   // Present
#define PTE_W 0x002   // Writeable
#define PTE_U 0x004   // User
#define PTE_PWT 0x008 // Write-Through
#define PTE_PCD 0x010 // Cache-Disable
#define PTE_A 0x020   // Accessed
#define PTE_D 0x040   // Dirty
#define PTE_PS 0x080  // Page Size
#define PTE_G 0x100   // Global

// Address in page table or page directory entry
#define PTE_ADDR(pte) ((phyaddr_t)(pte) & ~0xFFF)
#define PTE_FLAG(pte) ((size_t)(pte) & 0xFFF)

void map_laddr(phyaddr_t cr3, uintptr_t laddr, phyaddr_t paddr, u32 pte_attr);
phyaddr_t phy_kmalloc(size_t size);

size_t get_page_table_usage();
void setup_paging();
