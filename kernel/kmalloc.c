#include <minios/buddy.h>
#include <minios/console.h>
#include <minios/kmalloc.h>

malloced mal;
free_kmem kmem;

void malloced_insert(u32 addr, u32 size);

static u32 which_order(u32 size);

// added by mingxuan 2021-3-8
void kmem_init() {
    // kmem.kmem_table[0].addr=0x0;
    // kmem.kmem_table[0].size=4092;

    kmem.count = 0;
    mal.count = 0;
    kmem.total_mem_size = 0;
    kmem.current_mem_size = 0;
}

// added by wang 2021.6.8
u32 kmalloc_over4k(u32 size) {
    u32 order = which_order(size);
    memory_page_t *page = alloc_pages(kbud, order);
    u32 addr = pfn_to_phy(page_to_pfn(page));

    if (size < block_size[order]) { block_init(addr + size, addr + block_size[order], kbud); }

    malloced_insert(addr, size);

    kmem.total_mem_size += size;
    if (addr == 0) kprintf("kmalloc_over4k:kmalloc Error,no memory");

    return addr;
}

// added by wang 2021.6.8
u32 kfree_over4k(u32 addr) {
    u32 i, size = 0;

    for (i = 0; i < mal.count; i++) // 使用地址在分配表中查找已分配内存块
    {
        if (addr == mal.mtable[i].addr) {
            size = mal.mtable[i].size;

            break;
        }
    }
    if (i < mal.count) {
        mal.count--;

        for (; i < mal.count; i++) { mal.mtable[i] = mal.mtable[i + 1]; }
    } else {
        kprintf("kfree_over4k error");

        return -1; // 查找失败
    }

    kmem.total_mem_size -= size;
    block_init(addr, addr + size, kbud);

    return 0;
}

u32 get_kmalloc_size(u32 addr) {
    u32 i, size = 0;

    for (i = 0; i < mal.count; i++) // 使用地址在分配表中查找已分配内存块
    {
        if (addr == mal.mtable[i].addr) {
            size = mal.mtable[i].size;

            break;
        }
    }
    return size;
}

// added by wang 2021.6.8
static u32 which_order(u32 size) {
    u32 i;
    for (i = 0;; i++) {
        if (size <= block_size[i]) break;
    }
    return i;
}

void malloced_insert(u32 addr, u32 size) // 已经分配分区的记录表，供kfree使用
{
    u32 i = mal.count;

    mal.mtable[i].size = size;

    mal.mtable[i].addr = addr;

    mal.count++;

    if (mal.count >= MAX_MBLOCK) // add by wang 2021.3.25
        kprintf("kmalloc error:kmalloced table is full\n");
}
