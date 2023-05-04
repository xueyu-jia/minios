/*
 * @Author: Rong.Ding
 * @Date: 2023-01-13 23:13:25 
 * @Last Modified by: Yuanhong.Yu
 * @Last Modified time: 2023-01-13 23:14:29
 */
 //ported by sundong 2023.3.26

#ifndef _PAGING_H_
#define _PAGING_H_
#include "type.h"
#define PageDirBase 0x400000
#define PageTblBase	0x401000
#define PhyMallocBase 0x600000
#define PG_P 1
#define PG_USU 4
#define PG_RWW 2
#define PGSIZE		4096		// bytes mapped by a page

#define KERNBASE	0xC0000000
#define K_PHY2LIN(x)	((x)+KERNBASE)
#define _32M		0x2000000

#define KB		1024u
#define MB		(1024u * KB)
#define GB		(1024u * MB)

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
#define PGNUM(la)	(((uintptr_t) (la)) >> PTXSHIFT)

// page directory index
#define PDX(la)		((((uintptr_t) (la)) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(la)		((((uintptr_t) (la)) >> PTXSHIFT) & 0x3FF)

// offset in page
#define PGOFF(la)	(((uintptr_t) (la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o)	((void*) ((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define PTSIZE		(PGSIZE*NPTENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT		22		// log2(PTSIZE)

#define PTXSHIFT	12		// offset of PTX in a linear address
#define PDXSHIFT	22		// offset of PDX in a linear address

// Page table/directory entry flags.
#define PTE_P		0x001	// Present
#define PTE_W		0x002	// Writeable
#define PTE_U		0x004	// User
#define PTE_PWT		0x008	// Write-Through
#define PTE_PCD		0x010	// Cache-Disable
#define PTE_A		0x020	// Accessed
#define PTE_D		0x040	// Dirty
#define PTE_PS		0x080	// Page Size
#define PTE_G		0x100	// Global

// Address in page table or page directory entry
#define PTE_ADDR(pte)	((phyaddr_t) (pte) & ~0xFFF)
#define PTE_FLAG(pte)	((size_t) (pte) & 0xFFF)

struct ARDStruct
{
	u32 dwBaseAddrLow;
	u32 dwBaseAddrHigh;
	u32 dwLengthLow;
	u32 dwLengthHigh;
	u32 dwType;
}ARDStruct;

void paging(u32 MemChkBuf,u32 MCRNumber);
void map(u32 cr3, u32 size);

void
lin_mapping_phy(u32			cr3,
		uintptr_t		laddr,
		phyaddr_t		paddr,
		u32			pte_flag);
phyaddr_t pte_malloc_4k(void);
phyaddr_t phy_kmalloc(u32 size);
u32 get_mem_size(u32 MemChkBuf,u32 MCRNumber);

#endif