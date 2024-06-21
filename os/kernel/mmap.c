#include "type.h"
#include "const.h"
#include "fs.h"
#include "proc.h"
#include "mmap.h"
#include "pagetable.h"


/**
 * kmap: 将page对应的物理页映射到内核，返回可访问的线性地址
 * 对于
 * 如果无法建立映射，返回-1
*/
PUBLIC void * kmap(page *_page) {
    u32 phy = pfn_to_phy(page_to_pfn(_page));
    // disp_str("\nkmap:");
    // disp_int(phy);
    return kmapping_phy(phy);
}

/**
 * kunmap: 将page对应的物理页在内核页表的映射移除，
 * 内核段页表不进行任何操作
*/
PUBLIC void kunmap(page *_page) {
    u32 phy = pfn_to_phy(page_to_pfn(_page));
    // disp_str("\nkunmap:");
    // disp_int(phy);
    kunmapping_phy(phy);
}

// addr 查找的起始线性地址, 返回地址区间与已有页面的重叠页面数
PRIVATE u32 count_mapped_pages(LIN_MEMMAP* mmap, u32 start, u32 end) {
    struct vmem_area *vma;
    u32 nr_pages = 0;
    start &= 0xFFFFF000;
    end = UPPER_BOUND_4K(end);
    vma = find_vma(mmap, start);
    while(vma && end > vma->start) { // 后面存在已映射地址，找到第一个可用的空闲区域
        nr_pages += (min(vma->end, end) - max(vma->start, start)) >> PAGE_SHIFT;
        vma = list_next(&mmap->vma_map, vma, vma_list);
    }
    return nr_pages;
}

// addr 查找的起始线性地址, succ 返回时保存后继的vma, 
PRIVATE u32 get_unmapped_addr(LIN_MEMMAP* mmap, u32 addr, u32 len, struct vmem_area **succ) {
    struct vmem_area *vma;
    vma = find_vma(mmap, addr);
    while(vma && addr + len > vma->start) {// 后面存在已映射地址，找到第一个可用的空闲区域
        addr = vma->end;
        vma = list_next(&mmap->vma_map, vma, vma_list);
        // [addr, vma->start)为空闲区域，如果长度满足要求，那就是可用的
    }  
    *succ = vma;
    return addr;
}

/// @brief kern_mmap: mmap 核心实现 jiangfeng 2024.5
/// 尝试将进程p_proc的虚拟内存区域([addr,addr+len) 4K对齐,即不包含结束地址)映射到文件file
/// 如果没有配置OPT_MMU_COW写时复制标志，则在此处直接设置物理页和页表
/// 
/// @param p_proc 要进行内存映射的pcb,对于线程pcb则会使用其进程的memmap
/// @param file 要映射到内存的打开文件指针，如果file为NULL, 则设置匿名虚拟内存页
/// @param addr 映射区域的开始地址，如果此地址不可用且flag没有设置MAP_FIXED,则向高地址查找可用地址进行映射
/// @param len  映射区域的长度，单位字节(byte)
/// @param prot 映射区域的权限位，可通过与操作设置多种
/// PROT_READ可读， PROT_WRITE可写， PROT_EXEC可执行， PROT_NONE不可访问
/// @param flag 映射区域的标志位，MAP_SHARED与MAP_PRIVATE必须有且只能设置一种
/// MAP_SHARED: 此映射所有的改动与底层数据(如文件),其他进程共享 
/// MAP_PRIVATE: 此映射私有，所有的改动对文件系统和其他进程不可见
/// MAP_FIXED: 强制指定映射的开始地址固定为传入的addr, 
///     如果设置此标志位，而目标地址区域已经或部分被映射，则先调用munmap底层实现解除冲突的映射区域，再进行映射
///     如果没有设置此标志位，目标区域内存在映射，则从addr向高地址方向查找第一个可用的地址区域进行映射
/// @param pgoff 开始地址对应file的偏移量，以PAGE_SIZE(4096)为单位, 
///     如果file为NULL，类型为MAP_SHARED, pgoff则作为shmem的id,
///     file为NULL，类型为MAP_PRIVATE, pgoff由映射的虚拟地址决定，忽略传入的pgoff
/// @return 如果映射成功，返回实际映射的开始虚拟地址，否则返回-1
PUBLIC int kern_mmap(PROCESS* p_proc, struct file_desc *file, u32 addr, u32 len, 
    u32 prot, u32 flag, u32 pgoff) {
    len = UPPER_BOUND_4K(len);
    if(len == 0){
        return -1;
    }
    if((pgoff + (len >> PAGE_SHIFT)) < pgoff) {
        return -1; // len overflow
    }
    if(addr + len > KernelLinBase) {
        return -1;
    }
    if((!(flag & MAP_SHARED))^(!(flag & MAP_PRIVATE)) != 1){// MAP_SHARED MAP_PRIVATE必须二选一
        return -1;
    }
    if((flag & MAP_SHARED) && file){
        if((prot&PROT_WRITE) && (!(file->fd_mode & I_W))) {
            return -1; // request write but file cannot write
        }
        if((prot&PROT_EXEC) && (!(file->fd_mode & I_X))) {
            return -1; // request exec but file cannot execute
        }
    }
    LIN_MEMMAP* mmap = proc_memmap(p_proc);
    struct vmem_area *succ = NULL, *vma = NULL;
    lock_or_yield(&mmap->vma_lock);
    u32 covered = count_mapped_pages(mmap, addr, addr + len);
    // check prot flag
    if(covered && (flag & MAP_FIXED)) {
        release(&mmap->vma_lock);
        kern_munmap(p_proc, addr, len);
        lock_or_yield(&mmap->vma_lock);
    }
    // lock_or_yield(&p_proc->task.lock);
    u32 start_addr = get_unmapped_addr(mmap, addr, len, &succ);
    if(start_addr + len > KernelLinBase) { // 实际映射的区域不允许超出用户地址段
        goto fail_lock_out;
    }
    if(start_addr != addr && (flag & MAP_FIXED)) {
        disp_str("error: MAP_FIXED set but cant map param addr");
        goto fail_lock_out;
    }
    // link vma
    if(file){
        fget(file);
    }else if(flag & MAP_PRIVATE){
        pgoff = start_addr >> PAGE_SHIFT;
    }
    // 所有vma必须没有重叠，按照地址顺序排列于memmap.vma_map的链表
    // 为简化流程，mmap这里没有实现vma的merge
    // 对于mmap, merge操作不是必要的，但是对于某些对已有vma的操作，如mprotect必须实现merge
    vma = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
    vma->start = start_addr;
    vma->end = start_addr + len;
    vma->pgoff = pgoff;
    vma->file = file;
    vma->flags = prot | flag;
    list_init(&vma->vma_list);
    list_add_before(&vma->vma_list, (succ)?(&succ->vma_list):&mmap->vma_map);
    // 对于没有写时复制的mmap, 需要设置好物理页和页表
    release(&mmap->vma_lock);
    #ifndef OPT_MMU_COW
        prepare_vma(p_proc, mmap, vma);
    #endif
    // release(&p_proc->task.lock);
    return start_addr;
fail_lock_out:
    release(&mmap->vma_lock);
    // release(&p_proc->task.lock);
    return -1;
}

/// @brief 为当前进程映射虚拟地址空间
/// @param addr 映射区域的开始地址，如果此地址不可用且flag没有设置MAP_FIXED,则向高地址查找可用地址进行映射
/// @param len 映射区域的长度，单位字节(byte)
/// @param prot 映射区域的权限位，可通过与操作设置多种
/// PROT_READ可读， PROT_WRITE可写， PROT_EXEC可执行， PROT_NONE不可访问
/// @param flag 映射区域的标志位，MAP_SHARED与MAP_PRIVATE必须有且只能设置一种
/// MAP_SHARED: 此映射所有的改动与底层数据(如文件),其他进程共享 
/// MAP_PRIVATE: 此映射私有，所有的改动对文件系统和其他进程不可见
/// MAP_FIXED: 强制指定映射的开始地址固定为传入的addr, 
///     如果设置此标志位，而目标地址区域已经或部分被映射，则先调用munmap底层实现解除冲突的映射区域，再进行映射
///     如果没有设置此标志位，目标区域内存在映射，则从addr向高地址方向查找第一个可用的地址区域进行映射
/// @param fd 要映射的打开文件描述符，-1表示匿名映射(无文件关联)
/// @param offset 开始地址对应file的偏移量，4K(页面大小)对齐，以字节为单位, 
///     如果file为NULL，类型为MAP_SHARED, 则(offset/4096)作为shmem id查找映射,
///     file为NULL，类型为MAP_PRIVATE, 忽略传入的offset
/// @return 如果映射成功，返回实际映射的开始虚拟地址，否则返回-1
PUBLIC int do_mmap(u32 addr, u32 len, 
    u32 prot, u32 flag, int fd, u32 offset) {
    if(offset + UPPER_BOUND_4K(len) < offset || (addr & 0xFFF)) {
        return -1;
    }
    struct file_desc *file = NULL;
    if(fd != -1){
        if(fd < 0) return -1;
        file = p_proc_current->task.filp[fd];
        if(!file) return -1;
    }
    return kern_mmap(p_proc_current, file, addr, len, prot, flag, offset >> PAGE_SHIFT);
}

PRIVATE int split_vma(LIN_MEMMAP* mmap, struct vmem_area * vma, u32 addr, int new_below) {
    // copy
    struct vmem_area * new = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
    *new = *vma;
    list_init(&new->vma_list);
    if(new->file)
        fget(new->file);
    if(new_below) {
        new->end = addr;
    } else {
        new->start = addr;
        new->pgoff += ((addr - vma->start) >> PAGE_SHIFT);
    }
    if(new_below) {
        list_add_before(&new->vma_list, &vma->vma_list);
    } else {
        list_add_after(&new->vma_list, &vma->vma_list);
    }
}

PUBLIC int kern_munmap(PROCESS* p_proc, u32 start, u32 len) {
    len = UPPER_BOUND_4K(len);
    if(len == 0){
        return -1;
    }
    LIN_MEMMAP* mmap = proc_memmap(p_proc);
    lock_or_yield(&mmap->vma_lock);
    struct vmem_area *vma, *next, *last;
    u32 end = start + len;
    vma = find_vma(mmap, start);
	if (!vma) // no target region
		goto out;
    if(vma->start > end)
        goto out;// no need unmap
    if(start > vma->start) {
        split_vma(mmap, vma, start, 1);
    }
    last = find_vma(mmap, end);
    if(last && end > last->start) {
        split_vma(mmap, last, end, 1);
    }
    free_vmas(p_proc, mmap, vma, last);
out: 
    release(&mmap->vma_lock);
    return 0;
}

PUBLIC int do_munmap(u32 start, u32 len) {
    return kern_munmap(p_proc_current, start, len);
}