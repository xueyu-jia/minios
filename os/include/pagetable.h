#ifndef PAGETABLE_H
#define PAGETABLE_H
#include "type.h"
#include "const.h"
PUBLIC	u32 init_page_pte(u32 pid);
PUBLIC	int init_kernel_page();
PUBLIC	int init_proc_page(u32 pid);	//edit by visual 2016.4.28
PUBLIC	int kern_kmapping_phy(u32 phy_addr, u32 nr_pages);
PUBLIC 	int kern_kmapping_pop(u32 nr_pages);
PUBLIC	int lin_mapping_phy(u32 AddrLin,		  //线性地址
						   u32 phy_addr,	  //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
						   u32 pid,			  //进程pid						//edit by visual 2016.5.19
						   u32 pde_Attribute, //页目录中的属性位
						   u32 pte_Attribute); //页表中的属性位
PUBLIC	int lin_mapping_phy_nopid(u32 AddrLin,  //线性地址
								 u32 phy_addr, //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
								 u32 kernel_pde_addr_phy,
								 u32 pde_Attribute, //页目录中的属性位
								 u32 pte_Attribute);
PUBLIC	void free_all_phypage(u32 pid);
PUBLIC void free_seg_phypage(u32 pid, u8 type);
PUBLIC	void free_all_pagetbl(u32 pid);
PUBLIC void free_pagetbl(u32 pid, u32 AddrLin);
PUBLIC void free_pagedir(u32 pid);
PUBLIC	void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);//add by visual 2016.4.19
PUBLIC	u32 get_page_phy_addr(u32 pid,u32 AddrLin);//线性地址
PUBLIC	void clear_kernel_pagepte_low();		//add by visual 2016.5.12
PUBLIC	void clear_pte(u32 pid, u32 AddrLin);
void update_heap_limit(u32 pid, int tag);
u32 get_heap_limit(u32 pid);

					
#endif