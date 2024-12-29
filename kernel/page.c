/*************************************************************
 *页式管理相关代码 add by visual 2016.4.19
 **************************************************************/

#include <minios/buddy.h>
#include <minios/const.h>
#include <minios/memman.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/protect.h>
#include <minios/proto.h>
#include <minios/type.h>
#include <klib/spinlock.h>
#include <string.h>

// to determine if a page fault is reparable. added by xw, 18/6/11
u32 cr2_save;
u32 cr2_count = 0;
u32 kernel_pde_phy;
u32 nr_kmapping_pages = 0;
static u32 kmapping_pages[KernelLinMapMaxPage];
static struct spinlock kmap_lock;
/*======================================================================*
                           switch_pde			added by xw, 17/12/11
 *switch the page directory table after schedule() is called
 *======================================================================*/
void switch_pde() {
    cr3_ready = p_proc_current->task.cr3;
}

/// @brief 获取进程页表的物理页地址
/// @param pid 查询的进程号
/// @param AddrLin 查询的虚拟地址
/// @return
/// pte中保存的物理地址(此物理页可能不存在)，如果对应的页表不存在，返回NULL
u32 get_page_phy_addr(u32 pid, u32 AddrLin) {
    u32 pde_phy = get_pde_phy_addr(pid);
    if (!pde_phy || (pte_exist(pde_phy, AddrLin) == 0)) return (u32)NULL;
    u32 pte_phy = get_pte_phy_addr(pde_phy, AddrLin);
    if (!pte_phy) return (u32)NULL;
    return get_page_phy_addr_nopid(pte_phy, AddrLin);
}

/*======================================================================*
                          write_page_pde		add by visual 2016.4.28
 *======================================================================*/
void write_page_pde(u32 PageDirPhyAddr, // 页目录物理地址
                    u32 AddrLin,        // 线性地址
                    u32 TblPhyAddr, // 要填写的页表的物理地址（函数会进行4k对齐）
                    u32 Attribute)  // 属性
{                                   // 填写页目录
    (*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(AddrLin))) =
        (TblPhyAddr & 0xFFFFF000) | Attribute;
    // 进程页目录起始地址+每一项的大小*所属的项
}

/*======================================================================*
                          write_page_pte		add by visual 2016.4.28
 *======================================================================*/
void write_page_pte(u32 TblPhyAddr, // 页表物理地址
                    u32 AddrLin,    // 线性地址
                    u32 PhyAddr, // 要填写的物理页物理地址(任意的物理地址，函数会进行4k对齐)
                    u32 Attribute) // 属性
{                                  // 填写页目录，会添加属性
    (*((u32 *)K_PHY2LIN(TblPhyAddr) + get_pte_index(AddrLin))) = (PhyAddr & 0xFFFFF000) | Attribute;
    // 页表起始地址+一项的大小*所属的项
}

/*======================================================================*
                           init_proc_page		add by visual 2016.4.19
modifyed: jiangfeng 2024.4 *该函数只初始化了进程的高端（内核端）地址页表
 *======================================================================*/
int init_proc_page(u32 pid) { // 页表初始化函数

    u32 pde_addr_phy_temp;

    // pde_addr_phy_temp = do_kmalloc_4k();//为页目录申请一页
    pde_addr_phy_temp = phy_kmalloc_4k(); // 为页目录申请一页	//modified by mingxuan 2021-8-16

    memset((void *)K_PHY2LIN(pde_addr_phy_temp), 0,
           num_4K); // add by visual 2016.5.26

    if ((pde_addr_phy_temp & 0x3FF) != 0) // add by visual 2016.5.9
    {
        kprintf("init_proc_page Error:pde_addr_phy_temp");
        return -1;
    }

    proc_table[pid].task.cr3 = pde_addr_phy_temp; // 初始化了进程表中cr3寄存器变量，属性位暂时不管

    /*********************页表初始化部分*********************************/
    u32 kernel_pde_offset = KernelLinBase / num_4M * 4;
    memcpy((void *)(K_PHY2LIN(pde_addr_phy_temp) + kernel_pde_offset),
           (void *)(K_PHY2LIN(kernel_pde_phy) + kernel_pde_offset), num_4K - kernel_pde_offset);

    return 0;
}

// added by mingxuan 2021-8-25
int lin_mapping_phy_nopid(
    u32 AddrLin,  // 线性地址
    u32 phy_addr, // 物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
    u32 kernel_pde_addr_phy,
    u32 pde_Attribute, // 页目录中的属性位
    u32 pte_Attribute) // 页表中的属性位
{
    u32 pte_addr_phy;
    u32 pde_addr_phy = kernel_pde_addr_phy; // add by visual 2016.5.19

    if (0 == pte_exist(pde_addr_phy,
                       AddrLin)) { // 页表不存在，创建一个，并填进页目录中
        // pte_addr_phy = (u32)do_kmalloc_4k(); //为页表申请一页
        pte_addr_phy = (u32)phy_kmalloc_4k(); // 为页表申请一页	//modified by mingxuan 2021-8-16

        memset((void *)K_PHY2LIN(pte_addr_phy), 0,
               num_4K); // add by visual 2016.5.26

        if ((pte_addr_phy & 0x3FF) != 0) // add by visual 2016.5.9
        {
            kprintf("lin_mapping_phy Error:pte_addr_phy");
            return -1;
        }

        write_page_pde(pde_addr_phy,   // 页目录物理地址
                       AddrLin,        // 线性地址
                       pte_addr_phy,   // 页表物理地址
                       pde_Attribute); // 属性
    } else {                           // 页表存在，获取该页表物理地址
        pte_addr_phy = (*((u32 *)K_PHY2LIN(pde_addr_phy) + get_pde_index(AddrLin))) & 0xFFFFF000;
    }

    if (MAX_UNSIGNED_INT == phy_addr) // add by visual 2016.5.19
    {                                 // 由函数申请内存
        if (0 == phy_exist(pte_addr_phy,
                           AddrLin)) { // 无物理页，申请物理页并修改phy_addr
            if (u2ptr(AddrLin) >= K_PHY2LIN(0)) {
                // phy_addr = do_kmalloc_4k();//从内核物理地址申请一页
                phy_addr = phy_kmalloc_4k(); // 从内核物理地址申请一页	//modified by
                                             //  mingxuan 2021-8-16
            } else {
                // kprintf("%");
                // phy_addr = do_malloc_4k();//从用户物理地址空间申请一页
                phy_addr = phy_malloc_4k(); // 从用户物理地址空间申请一页
                                            ////modified by mingxuan 2021-8-14
            }
        } else {
            // 有物理页，什么也不做,直接返回，必须返回
            //  return 0;
            //  20240405
            //  对于已经存在的页表项可能需要更新权限属性，故不能直接返回
            phy_addr = get_page_phy_addr_nopid(pte_addr_phy, AddrLin);
        }
    } else { // 指定填写phy_addr
             // 不用修改phy_addr
    }

    if ((phy_addr & 0x3FF) != 0) {
        kprintf("lin_mapping_phy:phy_addr ERROR");
        return -1;
    }
    if (phy_addr == 0) { pte_Attribute &= ~PG_P; }

    write_page_pte(pte_addr_phy,   // 页表物理地址
                   AddrLin,        // 线性地址
                   phy_addr,       // 物理页物理地址
                   pte_Attribute); // 属性
    refresh_page_cache();

    return 0;
}

/*======================================================================*
 *                          lin_mapping_phy		add by visual 2016.5.9
 *将线性地址映射到物理地址上去,函数内部会分配物理地址
 *======================================================================*/
int map_laddr(
    u32 AddrLin,  // 线性地址
    u32 phy_addr, // 物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
    u32 pid,      // 进程pid						//edit
                  // by visual
                  //  2016.5.19
    u32 pde_Attribute, // 页目录中的属性位
    u32 pte_Attribute) // 页表中的属性位
{
    u32 pde_addr_phy = get_pde_phy_addr(pid); // add by visual 2016.5.19
    return lin_mapping_phy_nopid(AddrLin, phy_addr, pde_addr_phy, pde_Attribute, pte_Attribute);
}

// used for DMA/PCI etc. mapping kernel lin addr(>kernelsize) to phyaddr
// 在公共内核页表映射一个页面到物理地址phy_addr
int kmapping_phy(u32 phy_addr) {
    u32 lin_addr;
    lock_or_yield(&kmap_lock);
    if (phy_addr >= K_LIN2PHY(KernelLinMapBase)) { // 高端地址，3G+phy_addr无法直接访问
        if (nr_kmapping_pages == KernelLinMapMaxPage) {
            kprintf("no free kmapping space");
            release(&kmap_lock);
            return -1;
        }

        int index = 0;
        while (index < KernelLinMapMaxPage && kmapping_pages[index] != 0) index++;
        kmapping_pages[index] = phy_addr;
        lin_addr = KernelLinMapBase + (index << PAGE_SHIFT);
        nr_kmapping_pages++;
    } else if (phy_addr >= kernel_size) {
        lin_addr = ptr2u(K_PHY2LIN(phy_addr)); // 用户页物理地址，默认没有内核页表，需要写页表
    } else {
        release(&kmap_lock);
        return ptr2u(K_PHY2LIN(phy_addr)); // 无需映射，可直接访问
    }
    lin_mapping_phy_nopid(lin_addr, phy_addr, kernel_pde_phy, PG_P | PG_USS | PG_RWW,
                          PG_P | PG_USS | PG_RWW);
    release(&kmap_lock);
    return lin_addr;
}

int kunmapping_phy(u32 phy_addr) {
    u32 lin_addr = ptr2u(K_PHY2LIN(phy_addr));
    lock_or_yield(&kmap_lock);
    if (phy_addr >= K_LIN2PHY(KernelLinMapBase)) {
        if (nr_kmapping_pages == 0) {
            kprintf("no kmapping now");
            release(&kmap_lock);
            return -1;
        }
        int index = 0;
        while (index < KernelLinMapMaxPage && kmapping_pages[index] != phy_addr) index++;
        if (index == KernelLinMapMaxPage) {
            kprintf("no phy in kmapping");
            release(&kmap_lock);
            return -1;
        }
        kmapping_pages[index] = 0;
        lin_addr = KernelLinMapBase + (index << PAGE_SHIFT);
        nr_kmapping_pages--;
    } else if (phy_addr < kernel_size) {
        release(&kmap_lock);
        return 0;
    }
    u32 pte_phy_addr = get_pte_phy_addr(kernel_pde_phy, lin_addr);
    write_page_pte(pte_phy_addr, lin_addr, 0, 0);
    refresh_page_cache();
    release(&kmap_lock);
    return 0;
}

/*======================================================================*
 *                          clear_kernel_pagepte_low		add by visual
 *2016.5.12 将内核低端页表清除
 *======================================================================*/
void clear_kernel_pagepte_low() {
    u32 page_num = *(u32 *)PageTblNumAddr;
    memset((void *)(K_PHY2LIN(KernelPageTblAddr)), 0,
           4 * page_num); // 从内核页目录中清除内核页目录项前8项
    memset((void *)(K_PHY2LIN(KernelPageTblAddr + num_4K)), 0,
           num_4K * page_num); // 从内核页表中清除线性地址的低端映射关系
    refresh_page_cache();
}

// 清除页表项但不释放物理页
//  added by mingxuan 2021-1-4
void clear_pte(u32 pid, u32 AddrLin) {
    u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), AddrLin);
    write_page_pte(pte_phy_addr, AddrLin, 0, 0);
}

// 清除页目录表项但不释放页表
//  added by mingxuan 2021-1-4
void clear_pde(u32 pid, u32 AddrLin) {
    u32 pde_phy_addr = get_pde_phy_addr(pid);
    write_page_pde(pde_phy_addr, AddrLin, 0, 0);
}

// 释放页表，并清除该页表对应的页目录表项
//  added by mingxuan 2021-1-4
void free_pagetbl(u32 pid, u32 AddrLin) {
    u32 pagetbl_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), AddrLin);
    // do_kfree_4k(pagetbl_phy_addr);
    phy_kfree_4k(pagetbl_phy_addr); // modified by mingxuan 2021-8-16
    clear_pde(pid, AddrLin);
}

// 释放页目录表
//  added by mingxuan 2021-1-4
void free_pagedir(u32 pid) {
    u32 pagedir_phy_addr = get_pde_phy_addr(pid);
    // do_kfree_4k(pagedir_phy_addr);
    phy_kfree_4k(pagedir_phy_addr); // modified by mingxuan 2021-8-16
    p_proc_current->task.cr3 = 0;
}

// added by mingxuan 2021-1-7
// PUBLIC u32 set_seg_base(pid, type)
u32 get_seg_base(u32 pid, u32 type) // modified by mingxuan 2021-8-17
{
    if (type == MEMMAP_TEXT)
        return proc_table[pid].task.memmap.text_lin_base;
    else if (type == MEMMAP_DATA)
        return proc_table[pid].task.memmap.data_lin_base;
    else if (type == MEMMAP_VPAGE)
        return proc_table[pid].task.memmap.vpage_lin_base;
    else if (type == MEMMAP_HEAP)
        return proc_table[pid].task.memmap.heap_lin_base;
    else if (type == MEMMAP_STACK)
        return proc_table[pid].task.memmap.stack_lin_base;
    else if (type == MEMMAP_ARG)
        return proc_table[pid].task.memmap.arg_lin_base;
    else if (type == MEMMAP_KERNEL)
        return proc_table[pid].task.memmap.kernel_lin_base;
    return 0;
}

// added by mingxuan 2021-1-7
// PUBLIC u32 set_seg_limit(pid, type)
u32 get_seg_limit(u32 pid, u32 type) // modified by mingxuan 2021-8-17
{
    if (type == MEMMAP_TEXT)
        return proc_table[pid].task.memmap.text_lin_limit;
    else if (type == MEMMAP_DATA)
        return proc_table[pid].task.memmap.data_lin_limit;
    else if (type == MEMMAP_VPAGE)
        return proc_table[pid].task.memmap.vpage_lin_limit;
    else if (type == MEMMAP_HEAP)
        return proc_table[pid].task.memmap.heap_lin_limit;
    else if (type == MEMMAP_STACK)
        return proc_table[pid].task.memmap.stack_lin_limit;
    else if (type == MEMMAP_ARG)
        return proc_table[pid].task.memmap.arg_lin_limit;
    else if (type == MEMMAP_KERNEL)
        return proc_table[pid].task.memmap.kernel_lin_limit;
    return 0;
}

// 释放进程的某个段对应的所有页表
//  added by mingxuan 2021-1-7
void free_seg_pagetbl(u32 pid, u8 type) {
    u32 addr_lin;
    u32 low, high;

    // base = set_seg_base(pid, type);
    // limit = set_seg_limit(pid, type);
    // modified by mingxuan 2021-8-17
    low = get_seg_base(pid, type);
    high = get_seg_limit(pid, type);

    // 释放栈
    if (type == MEMMAP_STACK) // 栈段是从高低址向低地址生长的，释放时与其他不同
    {
        addr_lin = low;
        low = high;
        high = addr_lin;
    }
    low = LOWER_BOUND(low, num_4M);
    for (addr_lin = low; addr_lin < high; addr_lin += num_4M) {
        if (1 == pte_exist(get_pde_phy_addr(pid), addr_lin)) // 解决重复释放的问题
        {
            free_pagetbl(pid, addr_lin);
        }
    }
}

// 释放进程的全部页表(不能释放内核页表)
//  added by mingxuan 2021-1-6
void free_all_pagetbl(u32 pid) {
    free_seg_pagetbl(pid, MEMMAP_TEXT);
    free_seg_pagetbl(pid, MEMMAP_DATA); // 这里存在重复释放的问题
    free_seg_pagetbl(pid, MEMMAP_VPAGE);
    free_seg_pagetbl(pid, MEMMAP_HEAP);
    free_seg_pagetbl(pid, MEMMAP_STACK);
    free_seg_pagetbl(pid, MEMMAP_ARG);

    // 释放SharePage //释放SharePage,若释放会报错

    // 释放内核的映射 //不能释放内核页表
    //  free_seg_pagetbl(pid, MEMMAP_KERNEL);
}

void update_heap_limit(u32 pid, int tag)

{
    if (tag == 1) // heap_limit加上4K
    {
        if (proc_table[pid].task.tree_info.type == TYPE_PROCESS) {
            p_proc_current->task.memmap.heap_lin_limit += num_4K;
        } else {
            *(u32 *)(p_proc_current->task.memmap.heap_lin_limit) += num_4K;
        }
    } else if (tag == -1) // heap_limit减去4K
    {
        if (proc_table[pid].task.tree_info.type == TYPE_PROCESS) {
            p_proc_current->task.memmap.heap_lin_limit -= num_4K;
        } else {
            *(u32 *)(p_proc_current->task.memmap.heap_lin_limit) -= num_4K;
        }
    }
}

u32 get_heap_limit(u32 pid) {
    if (proc_table[pid].task.tree_info.type == TYPE_PROCESS) {
        return proc_table[pid].task.memmap.heap_lin_limit;
    } else {
        return *(u32 *)(proc_table[pid].task.memmap.heap_lin_limit);
    }
}

u32 get_pte(u32 addrLin) {
    u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(p_proc_current->task.pid), addrLin);
    u32 pte_index = get_pte_index(addrLin);
    return (*((u32 *)K_PHY2LIN(pte_phy_addr) + pte_index));
}
