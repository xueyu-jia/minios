#ifndef PAGETABLE_H
#define PAGETABLE_H
#include "type.h"
#include "const.h"
PUBLIC int init_kernel_page();
PUBLIC	int init_proc_page(u32 pid);	//edit by visual 2016.4.28
PUBLIC int kern_kmapping_phy(u32 phy_addr, u32 nr_pages);
PUBLIC int lin_mapping_phy(u32 AddrLin,		  //线性地址
						   u32 phy_addr,	  //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
						   u32 pid,			  //进程pid						//edit by visual 2016.5.19
						   u32 pde_Attribute, //页目录中的属性位
						   u32 pte_Attribute); //页表中的属性位
PUBLIC int lin_mapping_phy_nopid(u32 AddrLin,  //线性地址
								 u32 phy_addr, //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
								 u32 kernel_pde_addr_phy,
								 u32 pde_Attribute, //页目录中的属性位
								 u32 pte_Attribute);
PUBLIC void free_all_phypage(u32 pid);
PUBLIC 	void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);//add by visual 2016.4.19
PUBLIC	u32 get_pde_index(u32 AddrLin);//add by visual 2016.4.28
PUBLIC 	u32 get_pte_index(u32 AddrLin);
PUBLIC 	u32 get_pde_phy_addr(u32 pid);
PUBLIC 	u32 get_pte_phy_addr(u32 pid,u32 AddrLin);
PUBLIC  u32 get_page_phy_addr(u32 pid,u32 AddrLin);//线性地址
PUBLIC 	u32 pte_exist(u32 PageTblAddrPhy,u32 AddrLin);
PUBLIC 	u32 phy_exist(u32 PageTblPhyAddr,u32 AddrLin);
PUBLIC 	void write_page_pde(u32 PageDirPhyAddr,u32	AddrLin,u32 TblPhyAddr,u32 Attribute);
PUBLIC  void write_page_pte(	u32 TblPhyAddr,u32	AddrLin,u32 PhyAddr,u32 Attribute);
PUBLIC  u32 vmalloc(u32 size);
PUBLIC	void clear_kernel_pagepte_low();		//add by visual 2016.5.12

					
#endif