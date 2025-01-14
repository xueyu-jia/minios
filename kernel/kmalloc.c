#include <minios/kmalloc.h>
#include <minios/buddy.h>
#include <minios/console.h>
#include <minios/memman.h>

used_mem_info_t mal;
free_mem_info_t kmem;

static u32 which_order(u32 size) {
    u32 i;
    for (i = 0;; ++i) {
        if (size <= block_size[i]) break;
    }
    return i;
}

void kmem_init() {
    kmem.count = 0;
    mal.count = 0;
    kmem.total_mem_size = 0;
    kmem.current_mem_size = 0;
}

u32 kmalloc_over4k(u32 size) {
    u32 order = which_order(size);
    memory_page_t *page = alloc_pages(kbud, order);
    u32 addr = pfn_to_phy(page_to_pfn(page));

    if (size < block_size[order]) { block_init(addr + size, addr + block_size[order], kbud); }

    insert_used_memblk(addr, size);

    kmem.total_mem_size += size;
    if (addr == 0) { kprintf("kmalloc_over4k: kmalloc error, no memory"); }

    return addr;
}

u32 kfree_over4k(u32 addr) {
    u32 i, size = 0;

    // 使用地址在分配表中查找已分配内存块
    for (i = 0; i < mal.count; ++i) {
        if (addr == mal.mtable[i].addr) {
            size = mal.mtable[i].size;
            break;
        }
    }
    if (i < mal.count) {
        mal.count--;
        for (; i < mal.count; ++i) { mal.mtable[i] = mal.mtable[i + 1]; }
    } else {
        kprintf("kfree_over4k error");
        return -1; // 查找失败
    }

    kmem.total_mem_size -= size;
    block_init(addr, addr + size, kbud);

    return 0;
}

size_t get_kmalloc_size(u32 addr) {
    u32 i, size = 0;

    // 使用地址在分配表中查找已分配内存块
    for (i = 0; i < mal.count; ++i) {
        if (addr == mal.mtable[i].addr) {
            size = mal.mtable[i].size;
            break;
        }
    }
    return size;
}

// 已经分配分区的记录表，供 kfree 使用
void insert_used_memblk(u32 addr, u32 size) {
    u32 i = mal.count;
    mal.mtable[i].size = size;
    mal.mtable[i].addr = addr;
    mal.count++;
    if (mal.count >= MAX_MBLOCK) { kprintf("kmalloc error: kmalloced table is full\n"); }
}
