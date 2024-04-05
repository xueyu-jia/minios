#include "type.h"				
#include "const.h"

#define MEMMAN_FREES	4090		//32KB
//#define MEMMAN_ADDR	0x01ff0000	//存memman，31M960K
#define FMIBuff		0x007ff000	//loader中getFreeMemInfo返回值存放起始地址(7M1020K)
//#define KWALL		0x00600000
//#define WALL		0x00800000
//#define UWALL		0x01000000
//#define MEMSTART	0x00400000
//#define MEMEND		0x02000000
#define TEST		0x11223344
struct FREEINFO{
	u32 addr,size;
};					

struct MEMMAN{
	u32 frees,maxfrees,lostsize,losts;	//frees为当前空闲内存块数
	struct FREEINFO free[MEMMAN_FREES];	//空闲内存
};

PUBLIC u32 phy_kmalloc(u32 size);
PUBLIC u32 phy_kfree(u32 phy_addr);
PUBLIC u32 kern_kmalloc(u32 size);
PUBLIC u32 kern_kfree(u32 addr);
PUBLIC u32 phy_kmalloc_4k();
PUBLIC u32 phy_kfree_4k(u32 phy_addr);
PUBLIC void phy_mapping_4k(u32 phy_addr);
PUBLIC u32 kern_kmalloc_4k();
PUBLIC u32 kern_kfree_4k(u32 addr);
PUBLIC u32 phy_malloc_4k();
PUBLIC u32 phy_free_4k(u32 phy_addr);
PUBLIC int kern_mapping_4k(u32 AddrLin, u32 pid, u32 phyAddr, u32 pte_attribute);
PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute);
PUBLIC int ker_ufree_4k(u32 pid, u32 AddrLin);
