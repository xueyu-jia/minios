#include <malloc.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdbool.h>

static uint32_t vmalloc(uint32_t size);
static uint32_t pmalloc_4K();
static uint32_t vfree(uint32_t va, uint32_t size);

static void malloced_insert(uint32_t va, uint32_t size);

static uint32_t alloc_page();
static uint32_t alloc_vpage();
static uint32_t alloc_hpage();
static int free_page(uint32_t index);
static int free_vpage(uint32_t va);
static int free_ppage(uint32_t va);
void malloc_init();

// 用于malloc之前标志堆是否已经初始化，初始化了值为1，否则值为0.
bool malloc_initialized = false;

malloced mal;
free_vmem vmem;

vpage_list vplist;
heap_ptr hp;

void* malloc(size_t size) {
    if (!malloc_initialized) {
        malloc_init();
        malloc_initialized = true;
    }
    return (void*)vmalloc(size);
}

void free(void* ptr) {
    uint32_t i, size;
    uintptr_t va = (uintptr_t)ptr;

    // 用vaddr从已经malloc的内存块记录表中查找
    for (i = 0; i < mal.count; ++i) {
        if (va == mal.mtable[i].vaddr) {
            size = mal.mtable[i].size;

            break;
        }
    }
    if (i < mal.count) {
        mal.count--;

        for (; i < mal.count; ++i) { mal.mtable[i] = mal.mtable[i + 1]; }
    } else {
        printf("free error:memory block  didn't find\n");
        return; // 查找失败
    }

    if (size > SZ_4K) {
        uint32_t size_less_4K;

        size_less_4K = size % SZ_4K;

        if (size_less_4K != 0) {
            uint32_t tmp_vaddr = va + size - size_less_4K;

            vfree(tmp_vaddr, size_less_4K);
        }

        vfree(va, size - size_less_4K);
    } else {
        vfree(va, size);
    }
}

void malloc_init() {
    uint32_t base =
        (uint32_t)malloc_4k(); // 当malloc_4k系统调用传入参数为0时，返回内核态下堆底指针base;

    hp.heap_lin_base = base;

    hp.heap_lin_limit = base;

    free_4k((void*)base);

    vplist.vpage_count = 0; // added by wang 2021.3.25

    vmem.count = 0;

    mal.count = 0;
}

static uint32_t vmalloc(uint32_t size) {
    uint32_t i, va = 0;

    if (size >= SZ_4K) // added by wang 2021.4.6 ,edited by wang 2021.4.26
    {
        uint32_t page_num, size_less_4K;

        page_num = size / SZ_4K;

        size_less_4K = size % SZ_4K;

        if (size_less_4K != 0) {
            page_num++;

            va = alloc_hpage();

            for (i = 1; i < page_num; ++i) { alloc_hpage(); }

            uint32_t tmp_vaddr = va + (page_num - 1) * SZ_4K;

            vfree(tmp_vaddr + size_less_4K,
                  SZ_4K - size_less_4K); // 将剩余空闲内存挂在空闲分区表中

        } else {
            va = alloc_hpage();

            for (i = 1; i < page_num; ++i) { alloc_hpage(); }
        }

        malloced_insert(va, size);

        return va;

        // printf("error:it doesn't support size over 4K ");
        // return 0;
    }

    // 先从空闲分区表中查找合适的内存块（空闲分区小于4K）
    for (i = 0; i < vmem.count; ++i) {
        if (size <= vmem.vmem_table[i].size) // 找到合适的内存块并分配
        {
            va = vmem.vmem_table[i].vaddr;

            vmem.vmem_table[i].vaddr += size;

            vmem.vmem_table[i].size -= size;

            if (vmem.vmem_table[i].size == 0) {
                vmem.count--;

                for (; i < vmem.count; ++i) { vmem.vmem_table[i] = vmem.vmem_table[i + 1]; }
            }

            malloced_insert(va,
                            size); // 每分配一块，要将该块的信息记录在已经malloc的记录表中

            return va;
        }
    }

    // 没有找到合适的内存块，申请一个4K页面，并分配物理内存块，将剩余空闲块插入空闲表
    if (i >= vmem.count) {
        uint32_t page_addr;

        if ((page_addr = alloc_page()) > 0) // 分配4K逻辑页面
        {
            va = page_addr;

            vfree(va + size,
                  SZ_4K - size); // 将剩余空闲内存挂在空闲分区表中
        }

        else

            return 0;
    }

    malloced_insert(va, size);

    return va;
}

static void malloced_insert(uint32_t va,
                            uint32_t size) // 每分配一块，要将该块的信息记录在已经malloc的记录表中
{
    uint32_t i = mal.count;

    mal.mtable[i].size = size;

    mal.mtable[i].vaddr = va;

    mal.count++;

    if (mal.count >= MAX_MBLOCK) // added by wang 2021.3.25
        printf("malloc error: malloced table is full\n");
}

static uint32_t vfree(uint32_t va, uint32_t size) {
    // 释放
    size_t i, j, pageid;

    pageid = va / SZ_4K;

    for (i = 0; i < vmem.count; ++i) {
        if (vmem.vmem_table[i].vaddr > va) // FirstFit
        {
            break;
        }
    }

    if (i > 0) // 要释放的块的地址不是所有块中最低的，可能和前一块合并
    {
        // 如果前一块和当前块连续，并且同属一页，则合并
        if (vmem.vmem_table[i - 1].vaddr + vmem.vmem_table[i - 1].size == va &&
            vmem.vmem_table[i - 1].vaddr / SZ_4K == pageid) {
            vmem.vmem_table[i - 1].size += size;

            if (i < vmem.count) // 第i-1块非最后一块，则可能和后一块合并
            {
                // 和前面的块连续的同时还和后一块连续并且同属一页则合并
                if (va + size == vmem.vmem_table[i].vaddr &&
                    vmem.vmem_table[i].vaddr / SZ_4K == pageid) {
                    vmem.vmem_table[i - 1].size += vmem.vmem_table[i].size;

                    vmem.count--;

                    j = i;

                    for (; j < vmem.count; ++j) { vmem.vmem_table[j] = vmem.vmem_table[j + 1]; }
                }
            }

            if (vmem.vmem_table[i - 1].size >= SZ_4K) // 合并后的块大小等于4K，则释放该页面
            {
                free_page(i - 1);
            }

            return 0;
        }
    }

    if (i < vmem.count) //(i=0以及和前面的块不连续都走这个分支),
                        // 即和前面的块不连续或者前面无块，判断和后面的块是否连续
    {
        if (va + size == vmem.vmem_table[i].vaddr && vmem.vmem_table[i].vaddr / SZ_4K == pageid) {
            vmem.vmem_table[i].vaddr = va;

            vmem.vmem_table[i].size += size;

            if (vmem.vmem_table[i].size >= SZ_4K) // 当整个4K页面的分区都被释放时，释放相应的页面
            {
                free_page(i);
            }

            return 0;
        }
    }

    if (vmem.count < MAX_FREE) // 和前面后面的块都不连续且当前块数小于最大块数，直接插入块表
    {
        for (j = vmem.count; j > i; j--) { vmem.vmem_table[j] = vmem.vmem_table[j - 1]; }
        vmem.count++;

        if (vmem.count >= MAX_FREE) // added by wang 2021.3.25
            printf("malloc error: free partition table is full\n");

        vmem.vmem_table[i].vaddr = va; // 插入

        vmem.vmem_table[i].size = size;

        if (size >= SZ_4K) // 当整个4K页面的分区都被释放时，释放相应的页面
        {
            free_page(i);
        }

        return 0;
    }

    return -1;
}

static int free_page(uint32_t index) {
    uint32_t i, va, size; // size:added by wang 2021.4.6

    i = index;

    va = vmem.vmem_table[index].vaddr;
    size = vmem.vmem_table[index].size;

    for (; i < vmem.count - 1; ++i) { vmem.vmem_table[i] = vmem.vmem_table[i + 1]; }

    vmem.count--;

    // edited by wang 2021.4.26
    if (va + size == hp.heap_lin_limit) // 当要释放的逻辑页面在堆顶时，释放相应的物理页
    {
        uint32_t tmp_vaddr;

        for (tmp_vaddr = va; tmp_vaddr < va + size; tmp_vaddr += SZ_4K) { free_ppage(tmp_vaddr); }
    } else {
        uint32_t tmp_vaddr;

        for (tmp_vaddr = va; tmp_vaddr < va + size; tmp_vaddr += SZ_4K) { free_vpage(tmp_vaddr); }
    }

    /*
        else if(va + SZ_4K == hp.heap_lin_limit)
       //当要释放的逻辑页面在堆顶时，释放相应的物理页
        {

            free_ppage(va);

        }

        else    //否则只释放逻辑页（将其串在空闲表上），不释放物理页
        {

            free_vpage(va);

        }
    */
    return 0;
}

static int free_ppage(uint32_t va) ////当要释放的逻辑页面在堆顶时，释放相应的物理页
{
    hp.heap_lin_limit -= SZ_4K;

    free_4k((void*)va);

    if (vplist.vpage_count > 0) {
        while (vplist.vpage[vplist.vpage_count - 1] + SZ_4K ==
               hp.heap_lin_limit) // 循环更新堆指针limit,并释放堆顶页面

        {
            hp.heap_lin_limit -= SZ_4K;

            free_4k((void*)vplist.vpage[vplist.vpage_count - 1]);

            vplist.vpage[vplist.vpage_count - 1] = 0;

            vplist.vpage_count--;
        }
    }

    return 0;
}

static int free_vpage(uint32_t va) // First-Fit
{
    uint32_t i, j;

    for (i = 0; i < vplist.vpage_count; ++i) {
        if (vplist.vpage[i] > va) // First-Fit，方便循环释放物理页面

            break;
    }

    for (j = vplist.vpage_count; j > i; j--) { vplist.vpage[j] = vplist.vpage[j - 1]; }

    vplist.vpage[i] = va;

    vplist.vpage_count++;

    if (vplist.vpage_count >= MAX_VPAGE) // added by wang 2021.3.25
        printf("malloc error: vpage_list is full\n");

    return 0;
}

static uint32_t alloc_page() {
    uint32_t va;

    if (vplist.vpage_count >
        0) // 如果空闲页面表非空，则直接从表中分配页面（这些页面已经映射到相应的物理页面）
    {
        va = alloc_vpage();

    }

    else // 否则从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
    {
        va = alloc_hpage();
    }

    return va;
}

static uint32_t alloc_vpage() // 从空闲页面表中分配页面（这些页面已经映射到相应的物理页面）
{
    uint32_t i, va;

    va = vplist.vpage[0];

    vplist.vpage_count--;

    for (i = 0; i < vplist.vpage_count; ++i) { vplist.vpage[i] = vplist.vpage[i + 1]; }

    return va;
}

// 从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
static uint32_t alloc_hpage() {
    uint32_t va = hp.heap_lin_limit;
    hp.heap_lin_limit += SZ_4K;

    uint32_t vaddr_kernel = pmalloc_4K(); // 分配4K物理页面
    if (vaddr_kernel != va) { printf("malloc.c:error,va is no consistent\n"); }

    if (hp.heap_lin_limit > HeapLinLimitMAX) {
        printf("error: the heap is full\n");
        return 0;
    } else {
        return va;
    }
}

static uint32_t pmalloc_4K() {
    return (uint32_t)malloc_4k();
}

size_t total_mem_size() {
    return syscall(NR_total_mem_size);
}

void* malloc_4k() {
    return (void*)syscall(NR_malloc_4k);
}

void free_4k(void* ptr) {
    syscall(NR_free_4k, ptr);
}
