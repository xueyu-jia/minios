#ifndef PAGETABLE_H
#define PAGETABLE_H
#include "type.h"
#include "const.h"

PUBLIC int lin_mapping_phy_nopid(u32 AddrLin,  //线性地址
								 u32 phy_addr, //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
								 u32 kernel_pde_addr_phy,
								 u32 pde_Attribute, //页目录中的属性位
								 u32 pte_Attribute);
PUBLIC void free_all_phypage(u32 pid);
					
#endif