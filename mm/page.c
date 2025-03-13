#include <minios/page.h>
#include <minios/buddy.h>
#include <minios/memman.h>
#include <minios/layout.h>
#include <minios/proc.h>
#include <minios/console.h>
#include <minios/protect.h>
#include <minios/assert.h>
#include <minios/spinlock.h>
#include <minios/asm.h>
#include <klib/size.h>
#include <klib/stddef.h>
#include <string.h>
#include <limits.h>

// to determine if a page fault is reparable. added by xw, 18/6/11
u32 cr2_save;
u32 cr2_count = 0;
u32 kernel_pde_phy;
u32 nr_kmapping_pages = 0;
static u32 kmapping_pages[KernelLinMapMaxPage];
static spinlock_t kmap_lock;

void refresh_page_cache() {
    tlbflush();
}

void switch_pde() {
    cr3_ready = p_proc_current->task.cr3;
}

/// @brief 获取进程页表的物理页地址
/// @param pid 查询的进程号
/// @param va 查询的虚拟地址
/// @return
/// pte中保存的物理地址(此物理页可能不存在)，如果对应的页表不存在，返回NULL
u32 get_page_phy_addr(u32 pid, u32 va) {
    u32 pde_phy = get_pde_phy_addr(pid);
    if (!pde_phy || (pte_exist(pde_phy, va) == 0)) return (u32)NULL;
    u32 pte_phy = get_pte_phy_addr(pde_phy, va);
    if (!pte_phy) return (u32)NULL;
    return get_page_phy_addr_nopid(pte_phy, va);
}

/*======================================================================*
                          write_page_pde		add by visual 2016.4.28
 *======================================================================*/
void write_page_pde(u32 PageDirPhyAddr, // 页目录物理地址
                    u32 va,             // 线性地址
                    u32 TblPhyAddr, // 要填写的页表的物理地址（函数会进行4k对齐）
                    u32 Attribute)  // 属性
{                                   // 填写页目录
    (*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(va))) =
        (TblPhyAddr & 0xFFFFF000) | Attribute;
    // 进程页目录起始地址+每一项的大小*所属的项
}

/*======================================================================*
                          write_page_pte		add by visual 2016.4.28
 *======================================================================*/
void write_page_pte(u32 TblPhyAddr, // 页表物理地址
                    u32 va,         // 线性地址
                    u32 PhyAddr, // 要填写的物理页物理地址(任意的物理地址，函数会进行4k对齐)
                    u32 Attribute) // 属性
{                                  // 填写页目录，会添加属性
    (*((u32 *)K_PHY2LIN(TblPhyAddr) + get_pte_index(va))) = (PhyAddr & 0xFFFFF000) | Attribute;
    // 页表起始地址+一项的大小*所属的项
}

/*======================================================================*
                           init_proc_page		add by visual 2016.4.19
modifyed: jiangfeng 2024.4 *该函数只初始化了进程的高端（内核端）地址页表
 *======================================================================*/
int init_proc_page(u32 pid) { // 页表初始化函数

    u32 pde_addr_phy_temp;

    pde_addr_phy_temp = phy_kmalloc(SZ_4K);
    if (pde_addr_phy_temp == PHY_NULL) { return -1; }

    memset((void *)K_PHY2LIN(pde_addr_phy_temp), 0, SZ_4K);

    if ((pde_addr_phy_temp & 0x3FF) != 0) // add by visual 2016.5.9
    {
        kprintf("init_proc_page Error:pde_addr_phy_temp");
        return -1;
    }

    proc_table[pid].task.cr3 = pde_addr_phy_temp; // 初始化了进程表中cr3寄存器变量，属性位暂时不管

    /*********************页表初始化部分*********************************/
    u32 kernel_pde_offset = KernelLinBase / SZ_4M * 4;
    memcpy((void *)(K_PHY2LIN(pde_addr_phy_temp) + kernel_pde_offset),
           (void *)(K_PHY2LIN(kernel_pde_phy) + kernel_pde_offset), SZ_4K - kernel_pde_offset);

    return 0;
}

// added by mingxuan 2021-8-25
int lin_mapping_phy_nopid(
    u32 va, // 线性地址
    u32 pa, // 物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
    u32 kernel_pde_addr_phy,
    u32 pde_attr, // 页目录中的属性位
    u32 pte_attr) // 页表中的属性位
{
    u32 pte_addr_phy;
    u32 pde_addr_phy = kernel_pde_addr_phy; // add by visual 2016.5.19

    // 页表不存在，创建一个，并填进页目录中
    if (!pte_exist(pde_addr_phy, va)) {
        // 为页表申请一页
        pte_addr_phy = phy_kmalloc(SZ_4K);
        assert(pte_addr_phy != PHY_NULL);

        memset((void *)K_PHY2LIN(pte_addr_phy), 0, SZ_4K);

        if ((pte_addr_phy & 0x3FF) != 0) {
            kprintf("lin_mapping_phy Error:pte_addr_phy");
            return -1;
        }

        write_page_pde(pde_addr_phy, // 页目录物理地址
                       va,           // 线性地址
                       pte_addr_phy, // 页表物理地址
                       pde_attr);    // 属性
    } else {                         // 页表存在，获取该页表物理地址
        pte_addr_phy = (*((u32 *)K_PHY2LIN(pde_addr_phy) + get_pde_index(va))) & 0xFFFFF000;
    }

    if (MAX_UNSIGNED_INT == pa) // add by visual 2016.5.19
    {                           // 由函数申请内存
        if (0 == phy_exist(pte_addr_phy,
                           va)) { // 无物理页，申请物理页并修改phy_addr
            if (u2ptr(va) >= K_PHY2LIN(0)) {
                pa = phy_kmalloc(SZ_4K); // 从内核物理地址申请一页
            } else {
                pa = phy_malloc_4k(); // 从用户物理地址空间申请一页
            }
        } else {
            // 有物理页，什么也不做,直接返回，必须返回
            //  return 0;
            //  20240405
            //  对于已经存在的页表项可能需要更新权限属性，故不能直接返回
            pa = get_page_phy_addr_nopid(pte_addr_phy, va);
        }
    } else { // 指定填写phy_addr
             // 不用修改phy_addr
    }

    if ((pa & 0x3FF) != 0) {
        kprintf("lin_mapping_phy:pa ERROR");
        return -1;
    }
    if (pa == 0) { pte_attr &= ~PG_P; }

    write_page_pte(pte_addr_phy, // 页表物理地址
                   va,           // 线性地址
                   pa,           // 物理页物理地址
                   pte_attr);    // 属性
    refresh_page_cache();

    return 0;
}

/*======================================================================*
 *                          lin_mapping_phy		add by visual 2016.5.9
 *将线性地址映射到物理地址上去,函数内部会分配物理地址
 *======================================================================*/
// 物理地址若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
int map_laddr(u32 va,       // 线性地址
              u32 pa,       // 物理地址
              u32 pid,      // 进程pid
              u32 pde_attr, // 页目录中的属性位
              u32 pte_attr) // 页表中的属性位
{
    u32 pde_addr_phy = get_pde_phy_addr(pid); // add by visual 2016.5.19
    return lin_mapping_phy_nopid(va, pa, pde_addr_phy, pde_attr, pte_attr);
}

uintptr_t kmapping_phy(phyaddr_t pa) {
    assert(PGOFF(pa) == 0);
    const size_t pfn = phy_to_pfn(pa);
    if (pfn < nr_mmap_pages) {
        assert(pfn_to_page(pfn)->state != PAGESTATE_INVAL);
        assert(pfn_to_page(pfn)->state != PAGESTATE_CRITICAL);
    } else {
        //! NOTE: pa not in ram covered range, perhaps a hw mmio addr in high mem
    }

    uintptr_t va = ptr2u(NULL);
    spinlock_lock_or_yield(&kmap_lock);
    do {
        if (pa < K_LIN2PHY(KernelLinLimitMAX)) {
            va = ptr2u(K_PHY2LIN(pa));
            break;
        }
        if (nr_kmapping_pages == KernelLinMapMaxPage) {
            kprintf("error: no available slots in kernel mmap space\n");
            break;
        }
        int index = 0;
        while (index < KernelLinMapMaxPage && kmapping_pages[index] != 0) { ++index; }
        assert(index < KernelLinMapMaxPage);
        va = KernelLinMapBase + index * PGSIZE;
        const int retval = lin_mapping_phy_nopid(va, pa, kernel_pde_phy, PG_P | PG_USS | PG_RWW,
                                                 PG_P | PG_USS | PG_RWW);
        assert(retval == 0);
        kmapping_pages[index] = pa;
        ++nr_kmapping_pages;
    } while (0);
    spinlock_release(&kmap_lock);
    return va;
}

int kunmapping_phy(phyaddr_t pa) {
    assert(PGOFF(pa) == 0);

    bool has_pgobj = true;
    const size_t pfn = phy_to_pfn(pa);
    if (pfn < nr_mmap_pages) {
        assert(pfn_to_page(pfn)->state != PAGESTATE_INVAL);
        assert(pfn_to_page(pfn)->state != PAGESTATE_CRITICAL);
    } else {
        has_pgobj = false;
        //! NOTE: pa not in ram covered range, perhaps a hw mmio addr in high mem
    }

    int retval = 0;
    spinlock_lock_or_yield(&kmap_lock);
    do {
        if (pa < K_LIN2PHY(KernelLinLimitMAX)) { break; }
        if (nr_kmapping_pages == 0) {
            kprintf("warn: kunmapping_phy: pa 0x%p is not mapped yet\n", pa);
            break; //<! just ignore it since pa is "released" indeed
        }
        int index = 0;
        if (has_pgobj && pfn_to_page(pfn)->user_va) {
            index = (ptr2u(pfn_to_page(pfn)->user_va) - KernelLinMapBase) / PGSIZE;
            assert(kmapping_pages[index] == pa);
        } else {
            while (index < KernelLinMapMaxPage && kmapping_pages[index] != pa) { ++index; }
            assert(index < KernelLinMapMaxPage);
        }
        const uintptr_t va = KernelLinMapBase + index * PGSIZE;
        write_page_pte(get_pte_phy_addr(kernel_pde_phy, va), va, 0, 0);
        refresh_page_cache();
        kmapping_pages[index] = 0;
        --nr_kmapping_pages;
    } while (0);
    spinlock_release(&kmap_lock);
    return retval;
}

// 清除页表项但不释放物理页
//  added by mingxuan 2021-1-4
void clear_pte(u32 pid, u32 va) {
    u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), va);
    write_page_pte(pte_phy_addr, va, 0, 0);
}

// 清除页目录表项但不释放页表
//  added by mingxuan 2021-1-4
void clear_pde(u32 pid, u32 va) {
    u32 pde_phy_addr = get_pde_phy_addr(pid);
    write_page_pde(pde_phy_addr, va, 0, 0);
}

// 释放页表，并清除该页表对应的页目录表项
void free_pagetbl(u32 pid, u32 va) {
    u32 pagetbl_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), va);
    phy_kfree(pagetbl_phy_addr);
    clear_pde(pid, va);
}

// 释放页目录表
void free_pagedir(u32 pid) {
    u32 pagedir_phy_addr = get_pde_phy_addr(pid);
    phy_kfree(pagedir_phy_addr);
    p_proc_current->task.cr3 = 0;
}

u32 get_seg_base(u32 pid, u32 type) {
    switch (type) {
        case MEMMAP_TEXT: {
            return proc_table[pid].task.memmap.text_lin_base;
        } break;
        case MEMMAP_DATA: {
            return proc_table[pid].task.memmap.data_lin_base;
        } break;
        case MEMMAP_VPAGE: {
            return proc_table[pid].task.memmap.vpage_lin_base;
        } break;
        case MEMMAP_HEAP: {
            return proc_table[pid].task.memmap.heap_lin_base;
        } break;
        case MEMMAP_STACK: {
            return proc_table[pid].task.memmap.stack_lin_base;
        } break;
        case MEMMAP_ARG: {
            return proc_table[pid].task.memmap.arg_lin_base;
        } break;
        case MEMMAP_KERNEL: {
            return proc_table[pid].task.memmap.kernel_lin_base;
        } break;
    }
    return 0;
}

u32 get_seg_limit(u32 pid, u32 type) {
    switch (type) {
        case MEMMAP_TEXT: {
            return proc_table[pid].task.memmap.text_lin_limit;
        } break;
        case MEMMAP_DATA: {
            return proc_table[pid].task.memmap.data_lin_limit;
        } break;
        case MEMMAP_VPAGE: {
            return proc_table[pid].task.memmap.vpage_lin_limit;
        } break;
        case MEMMAP_HEAP: {
            return proc_table[pid].task.memmap.heap_lin_limit;
        } break;
        case MEMMAP_STACK: {
            return proc_table[pid].task.memmap.stack_lin_limit;
        } break;
        case MEMMAP_ARG: {
            return proc_table[pid].task.memmap.arg_lin_limit;
        } break;
        case MEMMAP_KERNEL: {
            return proc_table[pid].task.memmap.kernel_lin_limit;
        } break;
    }
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
    low = ROUNDDOWN(low, SZ_4M);
    for (addr_lin = low; addr_lin < high; addr_lin += SZ_4M) {
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

void update_heap_limit(u32 pid, int tag) {
    assert(tag == 1 || tag == -1);
    const ssize_t diff = tag == 1 ? SZ_4K : -SZ_4K;
    if (proc_table[pid].task.tree_info.type == TYPE_PROCESS) {
        p_proc_current->task.memmap.heap_lin_limit += diff;
    } else {
        *p_proc_current->task.memmap.heap_lin_limit_ref += diff;
    }
}

u32 get_heap_limit(u32 pid) {
    if (proc_table[pid].task.tree_info.type == TYPE_PROCESS) {
        return proc_table[pid].task.memmap.heap_lin_limit;
    } else {
        return *proc_table[pid].task.memmap.heap_lin_limit_ref;
    }
}

u32 get_pte(u32 addrLin) {
    u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(p_proc_current->task.pid), addrLin);
    u32 pte_index = get_pte_index(addrLin);
    return (*((u32 *)K_PHY2LIN(pte_phy_addr) + pte_index));
}

u32 get_pde_phy_addr(u32 pid) {
    if (proc_table[pid].task.cr3 == 0) {
        return (u32)NULL;
    } else {
        return ((proc_table[pid].task.cr3) & 0xfffff000);
    }
}

u32 get_pte_phy_addr(u32 pd_base, u32 va) {
    assert(pd_base != 0 && "invalid page directory address");
    return (*((u32 *)K_PHY2LIN(pd_base) + get_pde_index(va))) & 0xfffff000;
}

u32 get_page_phy_addr_nopid(u32 pt_base, u32 va) {
    assert(pt_base != 0 && "invalid page table address");
    return (*((u32 *)K_PHY2LIN(pt_base) + get_pte_index(va))) & 0xfffff000;
}

bool pte_exist(u32 pd_base, u32 va) {
    return ((*((u32 *)K_PHY2LIN(pd_base) + get_pde_index(va))) & PG_P) == PG_P;
}

bool phy_exist(u32 pt_base, u32 va) {
    return ((*((u32 *)K_PHY2LIN(pt_base) + get_pte_index(va))) & PG_P) == PG_P;
}
