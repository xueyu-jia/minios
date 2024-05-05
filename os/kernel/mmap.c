#include "type.h"
#include "const.h"
#include "fs.h"
#include "proc.h"
#include "memman.h"
#include "pagetable.h"


PUBLIC void * kmap(page *_page) {
    u32 phy = pfn_to_phy(page_to_pfn(_page));
    if(K_PHY2LIN(phy) < KernelLinBase + kernel_size) {
        return K_PHY2LIN(phy);
    }
    return kern_kmapping_phy(phy, 1);
}

PUBLIC void kunmap(page *_page) {
    u32 phy = pfn_to_phy(page_to_pfn(_page));
    if(K_PHY2LIN(phy) >= KernelLinBase + kernel_size) {
        kern_kmapping_pop(1);
    }
}

// addr 查找的起始线性地址, succ 返回时保存后继的vma
PRIVATE u32 count_mapped_pages(LIN_MEMMAP* mmap, u32 start, u32 end) {
    struct vmem_area *vma;
    u32 nr_pages = 0;
    start &= 0xFFFFF000;
    end = UPPER_BOUND_4K(end);
    vma = find_vma(mmap, start);
    if(vma) { // 后面存在已映射地址，找到第一个可用的空闲区域
        while(vma && end > vma->start) {
            nr_pages += (min(vma->end, end) - max(vma->start, start)) >> PAGE_SHIFT;
            vma = list_next(&mmap->vma_map, vma, vma_list);
        }
    }
    return nr_pages;
}

// addr 查找的起始线性地址, succ 返回时保存后继的vma
PRIVATE u32 get_unmapped_addr(LIN_MEMMAP* mmap, u32 addr, u32 len, struct vmem_area **succ) {
    struct vmem_area *vma;
    vma = find_vma(mmap, addr);
    if(vma) { // 后面存在已映射地址，找到第一个可用的空闲区域
        while(vma && addr + len > vma->start) {
            addr = vma->end;
            vma = list_next(&mmap->vma_map, vma, vma_list);
        }
        if(!vma) {
            return NULL;
        }
        // [addr, vma->start)为空闲区域
    }
    *succ = vma;
    return addr;
}

int kern_mmap(PROCESS* p_proc, struct file_desc *file, u32 addr, u32 len, 
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
    LIN_MEMMAP* mmap = proc_memmap(p_proc);
    struct vmem_area *succ = NULL, *vm = NULL;
    u32 covered = count_mapped_pages(mmap, addr, addr + len);
    if(covered && (flag & MAP_FIXED)) {
        return -1;
    }
    // check prot flag
    if(!((!(flag & MAP_SHARED))^(!(flag & MAP_PRIVATE)))){
        return -1;
    }
    if(!(prot & PROT_READ)) {
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
    if(covered) {
        kern_munmap(p_proc, addr, len);
    }
    lock_or_yield(&p_proc->task.lock);
    u32 start_addr = get_unmapped_addr(mmap, addr, len, &succ);
    // link vma
    if(file){
        fget(file);
    }else {
        pgoff = start_addr >> PAGE_SHIFT;
    }
    // 为简化流程，mmap这里没有实现vma的merge,所有vma必须没有重叠
    // 对于mmap, merge操作不是必要的，但是对于某些更进一步的操作，如mprotect必须实现merge
    vm = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
    vm->start = start_addr;
    vm->end = start_addr + len;
    vm->pgoff = pgoff;
    vm->file = file;
    vm->flags = prot | flag;
    list_init(&vm->vma_list);
    list_add_before(&vm->vma_list, (succ)?(&succ->vma_list):&mmap->vma_map);
    release(&p_proc->task.lock);
    return start_addr;
}

int do_mmap(u32 addr, u32 len, 
    u32 prot, u32 flag, int fd, u32 offset) {
    if(offset + UPPER_BOUND_4K(len) < offset || (addr & 0xFFF)) {
        return -1;
    }
    struct file_desc *file = NULL;
    if(fd != -1){
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

int kern_munmap(PROCESS* p_proc, u32 start, u32 len) {
    len = UPPER_BOUND_4K(len);
    if(len == 0){
        return -1;
    }
    LIN_MEMMAP* mmap = proc_memmap(p_proc);
    struct vmem_area *vma, *next, *last;
    u32 end = start + len;
    vma = find_vma(mmap, start);
	if (!vma) // no target region
		return 0;
    if(vma->start > end)
        return 0;
    lock_or_yield(&p_proc->task.lock);
    if(start > vma->start) {
        split_vma(mmap, vma, start, 1);
    }
    last = find_vma(mmap, end);
    if(last && end > last->start) {
        split_vma(mmap, last, end, 1);
    }
    free_vmas(p_proc, mmap, vma, last);
    release(&p_proc->task.lock);
    return 0;
}

int do_munmap(u32 start, u32 len) {
    return kern_munmap(p_proc_current, start, len);
}