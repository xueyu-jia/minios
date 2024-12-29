/*************************************************************
 * 用户态下小内存管理malloc相关代码    add by wang  2021.3.3
 **************************************************************/

#include <malloc.h>
#include <stdio.h>
#include <stddef.h>

void* malloc_4k();
int free_4k(void* ptr);

// 核心函数
static uint32_t vmalloc(uint32_t size);               // 分配逻辑内存块
static uint32_t pmalloc_4K();                         // 分配物理4K页面
static uint32_t vfree(uint32_t vaddr, uint32_t size); // 释放逻辑内存块

static void malloced_insert(uint32_t vaddr,
                            uint32_t size); // 为malloc记录表插入一项记录

static uint32_t alloc_page();
static uint32_t alloc_vpage();
static uint32_t alloc_hpage();
static int free_page(uint32_t index); // 当vfree后整个4K页面都空时释放该页
static int free_vpage(uint32_t vaddr);
static int free_ppage(uint32_t vaddr);
void malloc_init();

int malloc_initialized = 0; // 用于malloc之前标志堆是否已经初始化，初始化了值为1，否则值为0.
// malloc_page ptable;
malloced mal;
free_vmem vmem;

vpage_list vplist;
heap_ptr hp;

uint32_t malloc(uint32_t size) // 为用户分配小内存
{
    if (!malloc_initialized) // malloc之前先判断是否已经初始化
    {
        malloc_init();

        malloc_initialized = 1;
    }

    uint32_t vaddr = vmalloc(size);

    if (vaddr == 0) { printf("malloc failed!\n"); }

    return vaddr;
}

uint32_t free(uint32_t vaddr) // 用户分配的小内存的释放
{
    uint32_t i, size;

    for (i = 0; i < mal.count; i++) // 用vaddr从已经malloc的内存块记录表中查找
    {
        if (vaddr == mal.mtable[i].vaddr) {
            size = mal.mtable[i].size;

            break;
        }
    }
    if (i < mal.count) {
        mal.count--;

        for (; i < mal.count; i++) { mal.mtable[i] = mal.mtable[i + 1]; }
    } else {
        printf("free error:memory block  didn't find\n");
        return 0; // 查找失败
    }

    if (size > num_4K) // added by wang 2021.4.25
    {
        uint32_t size_less_4K;

        size_less_4K = size % num_4K;

        if (size_less_4K != 0) {
            uint32_t tmp_vaddr = vaddr + size - size_less_4K;

            vfree(tmp_vaddr, size_less_4K);
        }

        vfree(vaddr, size - size_less_4K);
    } else
        vfree(vaddr, size);

    return 0;
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

/*
uint32_t sys_malloc_4k(uint32_t vaddr)
{
    //此处为模拟，具体实现初步设想如下

    //paddr=alloc_pages(0);
    //line_mapping_phy2();


    static uint32_t paddr = 0x00800000 - num_4K;
    paddr += num_4K;
    //printf("sys_malloc:%x\n",paddr);
    return paddr;
}



uint32_t sys_free_4k(uint32_t paddr)
{
    //此处为做任何处理，具体实现初步设想如下

    //page freepage;
    //freepage.paddr=paddr;
    //free_pages(freepage,0);
    //remove_pte();

    return 0;
}

*/

static uint32_t vmalloc(uint32_t size) {
    uint32_t i, vaddr = 0;

    if (size >= num_4K) // added by wang 2021.4.6 ,edited by wang 2021.4.26
    {
        uint32_t page_num, size_less_4K;

        page_num = size / num_4K;

        size_less_4K = size % num_4K;

        if (size_less_4K != 0) {
            page_num++;

            vaddr = alloc_hpage();

            for (i = 1; i < page_num; i++) { alloc_hpage(); }

            uint32_t tmp_vaddr = vaddr + (page_num - 1) * num_4K;

            vfree(tmp_vaddr + size_less_4K,
                  num_4K - size_less_4K); // 将剩余空闲内存挂在空闲分区表中

        } else {
            vaddr = alloc_hpage();

            for (i = 1; i < page_num; i++) { alloc_hpage(); }
        }

        malloced_insert(vaddr, size);

        return vaddr;

        // printf("error:it doesn't support size over 4K ");
        // return 0;
    }

    // 先从空闲分区表中查找合适的内存块（空闲分区小于4K）
    for (i = 0; i < vmem.count; i++) {
        if (size <= vmem.vmem_table[i].size) // 找到合适的内存块并分配
        {
            vaddr = vmem.vmem_table[i].vaddr;

            vmem.vmem_table[i].vaddr += size;

            vmem.vmem_table[i].size -= size;

            if (vmem.vmem_table[i].size == 0) {
                vmem.count--;

                for (; i < vmem.count; i++) { vmem.vmem_table[i] = vmem.vmem_table[i + 1]; }
            }

            malloced_insert(vaddr,
                            size); // 每分配一块，要将该块的信息记录在已经malloc的记录表中

            return vaddr;
        }
    }

    // 没有找到合适的内存块，申请一个4K页面，并分配物理内存块，将剩余空闲块插入空闲表
    if (i >= vmem.count) {
        uint32_t page_addr;

        if ((page_addr = alloc_page()) > 0) // 分配4K逻辑页面
        {
            vaddr = page_addr;

            vfree(vaddr + size,
                  num_4K - size); // 将剩余空闲内存挂在空闲分区表中
        }

        else

            return 0;
    }

    malloced_insert(vaddr, size);

    return vaddr;
}

static void malloced_insert(uint32_t vaddr,
                            uint32_t size) // 每分配一块，要将该块的信息记录在已经malloc的记录表中
{
    uint32_t i = mal.count;

    mal.mtable[i].size = size;

    mal.mtable[i].vaddr = vaddr;

    mal.count++;

    if (mal.count >= MAX_MBLOCK) // added by wang 2021.3.25
        printf("malloc error: malloced table is full\n");
}

static uint32_t vfree(uint32_t vaddr, uint32_t size) {
    // 释放
    size_t i, j, pageid;

    pageid = vaddr / num_4K;

    for (i = 0; i < vmem.count; i++) {
        if (vmem.vmem_table[i].vaddr > vaddr) // FirstFit
        {
            break;
        }
    }

    if (i > 0) // 要释放的块的地址不是所有块中最低的，可能和前一块合并
    {
        // 如果前一块和当前块连续，并且同属一页，则合并
        if (vmem.vmem_table[i - 1].vaddr + vmem.vmem_table[i - 1].size == vaddr &&
            vmem.vmem_table[i - 1].vaddr / num_4K == pageid) {
            vmem.vmem_table[i - 1].size += size;

            if (i < vmem.count) // 第i-1块非最后一块，则可能和后一块合并
            {
                // 和前面的块连续的同时还和后一块连续并且同属一页则合并
                if (vaddr + size == vmem.vmem_table[i].vaddr &&
                    vmem.vmem_table[i].vaddr / num_4K == pageid) {
                    vmem.vmem_table[i - 1].size += vmem.vmem_table[i].size;

                    vmem.count--;

                    j = i;

                    for (; j < vmem.count; j++) { vmem.vmem_table[j] = vmem.vmem_table[j + 1]; }
                }
            }

            if (vmem.vmem_table[i - 1].size >= num_4K) // 合并后的块大小等于4K，则释放该页面
            {
                free_page(i - 1);
            }

            return 0;
        }
    }

    if (i < vmem.count) //(i=0以及和前面的块不连续都走这个分支),
                        // 即和前面的块不连续或者前面无块，判断和后面的块是否连续
    {
        if (vaddr + size == vmem.vmem_table[i].vaddr &&
            vmem.vmem_table[i].vaddr / num_4K == pageid) {
            vmem.vmem_table[i].vaddr = vaddr;

            vmem.vmem_table[i].size += size;

            if (vmem.vmem_table[i].size >= num_4K) // 当整个4K页面的分区都被释放时，释放相应的页面
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

        vmem.vmem_table[i].vaddr = vaddr; // 插入

        vmem.vmem_table[i].size = size;

        if (size >= num_4K) // 当整个4K页面的分区都被释放时，释放相应的页面
        {
            free_page(i);
        }

        return 0;
    }

    return -1;
}

static int free_page(uint32_t index) {
    uint32_t i, vaddr, size; // size:added by wang 2021.4.6

    i = index;

    vaddr = vmem.vmem_table[index].vaddr;
    size = vmem.vmem_table[index].size;

    for (; i < vmem.count - 1; i++) { vmem.vmem_table[i] = vmem.vmem_table[i + 1]; }

    vmem.count--;

    // edited by wang 2021.4.26
    if (vaddr + size == hp.heap_lin_limit) // 当要释放的逻辑页面在堆顶时，释放相应的物理页
    {
        uint32_t tmp_vaddr;

        for (tmp_vaddr = vaddr; tmp_vaddr < vaddr + size; tmp_vaddr += num_4K) {
            free_ppage(tmp_vaddr);
        }
    } else {
        uint32_t tmp_vaddr;

        for (tmp_vaddr = vaddr; tmp_vaddr < vaddr + size; tmp_vaddr += num_4K) {
            free_vpage(tmp_vaddr);
        }
    }

    /*
        else if(vaddr + num_4K == hp.heap_lin_limit)
       //当要释放的逻辑页面在堆顶时，释放相应的物理页
        {

            free_ppage(vaddr);

        }

        else    //否则只释放逻辑页（将其串在空闲表上），不释放物理页
        {

            free_vpage(vaddr);

        }
    */
    return 0;
}

static int free_ppage(uint32_t vaddr) ////当要释放的逻辑页面在堆顶时，释放相应的物理页
{
    hp.heap_lin_limit -= num_4K;

    free_4k((void*)vaddr);

    if (vplist.vpage_count > 0) {
        while (vplist.vpage[vplist.vpage_count - 1] + num_4K ==
               hp.heap_lin_limit) // 循环更新堆指针limit,并释放堆顶页面

        {
            hp.heap_lin_limit -= num_4K;

            free_4k((void*)vplist.vpage[vplist.vpage_count - 1]);

            vplist.vpage[vplist.vpage_count - 1] = 0;

            vplist.vpage_count--;
        }
    }

    return 0;
}

static int free_vpage(uint32_t vaddr) // First-Fit
{
    uint32_t i, j;

    for (i = 0; i < vplist.vpage_count; i++) {
        if (vplist.vpage[i] > vaddr) // First-Fit，方便循环释放物理页面

            break;
    }

    for (j = vplist.vpage_count; j > i; j--) { vplist.vpage[j] = vplist.vpage[j - 1]; }

    vplist.vpage[i] = vaddr;

    vplist.vpage_count++;

    if (vplist.vpage_count >= MAX_VPAGE) // added by wang 2021.3.25
        printf("malloc error: vpage_list is full\n");

    return 0;
}

static uint32_t alloc_page() {
    uint32_t vaddr;

    if (vplist.vpage_count >
        0) // 如果空闲页面表非空，则直接从表中分配页面（这些页面已经映射到相应的物理页面）
    {
        vaddr = alloc_vpage();

    }

    else // 否则从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
    {
        vaddr = alloc_hpage();
    }

    return vaddr;
}

static uint32_t alloc_vpage() // 从空闲页面表中分配页面（这些页面已经映射到相应的物理页面）
{
    uint32_t i, vaddr;

    vaddr = vplist.vpage[0];

    vplist.vpage_count--;

    for (i = 0; i < vplist.vpage_count; i++) { vplist.vpage[i] = vplist.vpage[i + 1]; }

    return vaddr;
}

static uint32_t alloc_hpage() // 从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
{
    uint32_t vaddr = hp.heap_lin_limit;

    void* malloc_4k();

    hp.heap_lin_limit += num_4K;

    uint32_t vaddr_kernel = pmalloc_4K(); // 分配4K物理页面
    if (vaddr_kernel != vaddr) printf("malloc.c:error,vaddr is no consistent\n");

    if (hp.heap_lin_limit > HeapLinLimitMAX) {
        printf("error:the heap is full\n");
        return 0;
    } else

        return vaddr;
}

static uint32_t pmalloc_4K() {
    uint32_t vaddr = (uint32_t)malloc_4k();

    return vaddr;
}

// test
void scan_heap_ptr() {
    printf("base=0x%p limit=0x%p\n", (void*)hp.heap_lin_base, (void*)hp.heap_lin_limit);
}

void scan_page_list() {
    uint32_t i;
    printf("page_count=%d\n", vplist.vpage_count);
    for (i = 0; i < vplist.vpage_count; i++) { printf("  addr=0x%p", (void*)vplist.vpage[i]); }
    printf("\n");
}

void scan_vtable() {
    uint32_t i;
    for (i = 0; i < vmem.count; i++) {
        printf("%d:  addr=0x%p  size=%d\n", i, (void*)vmem.vmem_table[i].vaddr,
               vmem.vmem_table[i].size);
        if (i % 2 == 1) {
            printf("\n");
        } else {
            printf("  ");
        }
    }
    if (i % 2 == 1) { printf("\n"); }
}

void scan_malloc_table() {
    uint32_t i;
    for (i = 0; i < mal.count; i++) {
        printf("%d:  addr=0x%p  size=%d\n", i, (void*)mal.mtable[i].vaddr, mal.mtable[i].size);
        if (i % 2 == 1) {
            printf("\n");
        } else {
            printf("  ");
        }
    }
    if (i % 2 == 1) { printf("\n"); }
}
