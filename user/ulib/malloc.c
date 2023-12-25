/*************************************************************
* 用户态下小内存管理malloc相关代码    add by wang  2021.3.3
**************************************************************/


#include "malloc.h"
#include "type.h"

// #include "../include/malloc.h"
// #include "../include/type.h"


//核心函数
static u32 vmalloc(u32 size);    //分配逻辑内存块
static u32 pmalloc_4K();  //分配物理4K页面
static u32 vfree(u32 vaddr, u32 size);   //释放逻辑内存块


static void malloced_insert(u32 vaddr,u32 size);    //为malloc记录表插入一项记录

static u32 alloc_page();
static u32 alloc_vpage();
static u32 alloc_hpage();
static int free_page(u32 index);     //当vfree后整个4K页面都空时释放该页
static int free_vpage(u32 vaddr);
static int free_ppage(u32 vaddr);

int malloc_initialized=0;          //用于malloc之前标志堆是否已经初始化，初始化了值为1，否则值为0.
//malloc_page ptable;
malloced mal;
free_vmem vmem;

vpage_list vplist;
heap_ptr hp;


u32 malloc(u32 size)   //为用户分配小内存
{
    
    if(!malloc_initialized)  //malloc之前先判断是否已经初始化
    {

        malloc_init();

        malloc_initialized = 1;

    }

    u32 vaddr = vmalloc(size);

    if(vaddr==0)
        udisp_str("malloc failed!\n");

    return vaddr;

}

u32 free(u32 vaddr)          //用户分配的小内存的释放
{

    u32 i,size;

    for(i = 0; i < mal.count; i++)  //用vaddr从已经malloc的内存块记录表中查找
    {

        if(vaddr == mal.mtable[i].vaddr)
        {

            size = mal.mtable[i].size;

            break;
        }

    }
    if(i < mal.count)
    {

        mal.count--;

        for(; i < mal.count; i++)
        {

            mal.mtable[i] = mal.mtable[i+1];

        }
    }
    else
    {
        udisp_str("free error:memory block  didn't find\n");
        return 0;//查找失败

    }

    if(size > num_4K)                    //added by wang 2021.4.25
    {
        u32 size_less_4K;

        size_less_4K = size % num_4K;

        if(size_less_4K!=0)
        {
            u32 tmp_vaddr = vaddr + size - size_less_4K;

            vfree(tmp_vaddr,size_less_4K);
        }

        vfree(vaddr,size-size_less_4K);
    }
    else
        vfree(vaddr,size);

    return 0;
}


void malloc_init()
{

    u32 base = malloc_4k();  //当malloc_4k系统调用传入参数为0时，返回内核态下堆底指针base;

    hp.heap_lin_base = base;

    hp.heap_lin_limit = base;

    free_4k(base);

    vplist.vpage_count = 0;               //added by wang 2021.3.25

    vmem.count = 0;

    mal.count = 0;
}


/*
u32 sys_malloc_4k(u32 vaddr)
{
    //此处为模拟，具体实现初步设想如下

    //paddr=alloc_pages(0);
    //line_mapping_phy2();


    static u32 paddr = 0x00800000 - num_4K;
    paddr += num_4K;
    //printf("sys_malloc:%x\n",paddr);
    return paddr;
}



u32 sys_free_4k(u32 paddr)
{
    //此处为做任何处理，具体实现初步设想如下

    //page freepage;
    //freepage.paddr=paddr;
    //free_pages(freepage,0);
    //remove_pte();

    return 0;
}

*/



static u32 vmalloc(u32 size)
{
    u32 i,vaddr;

    if(size >= num_4K)   //added by wang 2021.4.6 ,edited by wang 2021.4.26
    {
        u32 page_num,size_less_4K;

        page_num = size / num_4K;

        size_less_4K = size % num_4K;

        if(size_less_4K!= 0)
        {
            page_num++;

            vaddr=alloc_hpage();

            for(i = 1; i < page_num; i++)
            {

                alloc_hpage();

            }

            u32 tmp_vaddr = vaddr + (page_num-1) * num_4K;

            vfree(tmp_vaddr+size_less_4K,num_4K - size_less_4K);  //将剩余空闲内存挂在空闲分区表中

        }
        else
        {
            vaddr=alloc_hpage();

            for(i = 1; i < page_num; i++)
            {

                alloc_hpage();

            }
        }

        malloced_insert(vaddr,size);

        return vaddr;


        //udisp_str("error:it doesn't support size over 4K ");
        //return 0;
    }

    for(i = 0; i < vmem.count; i++)  //先从空闲分区表中查找合适的内存块（空闲分区小于4K）
    {

        if(size <= vmem.vmem_table[i].size)//找到合适的内存块并分配
        {

            vaddr = vmem.vmem_table[i].vaddr;

            vmem.vmem_table[i].vaddr += size;

            vmem.vmem_table[i].size -= size;

            if(vmem.vmem_table[i].size == 0)
            {

                vmem.count--;

                for(; i < vmem.count; i++)
                {

                    vmem.vmem_table[i] = vmem.vmem_table[i+1];

                }
            }

            malloced_insert(vaddr,size);  //每分配一块，要将该块的信息记录在已经malloc的记录表中

            return vaddr;

        }

    }

    if(i >= vmem.count)//没有找到合适的内存块，申请一个4K页面，并分配物理内存块，将剩余空闲块插入空闲表
    {

        u32 page_addr;

        if((page_addr = alloc_page())>0)  //分配4K逻辑页面
        {

            vaddr = page_addr;

            vfree(vaddr + size,num_4K - size);  //将剩余空闲内存挂在空闲分区表中
        }

        else

            return 0;

    }

    malloced_insert(vaddr,size);

    return vaddr;

}



static void malloced_insert(u32 vaddr,u32 size)//每分配一块，要将该块的信息记录在已经malloc的记录表中
{
    u32 i = mal.count;

    mal.mtable[i].size = size;

    mal.mtable[i].vaddr = vaddr;

    mal.count++;

    if(mal.count >= MAX_MBLOCK)                                    //added by wang 2021.3.25
        udisp_str("malloc error: malloced table is full\n");

}






static u32 vfree(u32 vaddr, u32 size)
{
    //释放
    int i,j,pageid;

    pageid = vaddr / num_4K;

    for(i = 0; i < vmem.count; i++)
    {

        if(vmem.vmem_table[i].vaddr > vaddr)//FirstFit
        {
            break;
        }
    }

    if(i > 0)//要释放的块的地址不是所有块中最低的，可能和前一块合并
    {
        //如果前一块和当前块连续，并且同属一页，则合并
        if(vmem.vmem_table[i-1].vaddr + vmem.vmem_table[i-1].size == vaddr && vmem.vmem_table[i-1].vaddr / num_4K == pageid)
        {

            vmem.vmem_table[i-1].size += size;

            if(i < vmem.count)//第i-1块非最后一块，则可能和后一块合并
            {

                //和前面的块连续的同时还和后一块连续并且同属一页则合并
                if(vaddr + size == vmem.vmem_table[i].vaddr && vmem.vmem_table[i].vaddr / num_4K == pageid)
                {

                    vmem.vmem_table[i-1].size += vmem.vmem_table[i].size;

                    vmem.count--;

                    j = i;

                    for(; j < vmem.count; j++)
                    {

                        vmem.vmem_table[j] = vmem.vmem_table[j+1];

                    }

                }

            }

            if(vmem.vmem_table[i-1].size >= num_4K)//合并后的块大小等于4K，则释放该页面
            {

                free_page(i-1);

            }

            return 0;
        }

    }

    if(i < vmem.count)//(i=0以及和前面的块不连续都走这个分支),
        //即和前面的块不连续或者前面无块，判断和后面的块是否连续
    {

        if(vaddr + size == vmem.vmem_table[i].vaddr && vmem.vmem_table[i].vaddr/num_4K == pageid)
        {

            vmem.vmem_table[i].vaddr = vaddr;

            vmem.vmem_table[i].size += size;

            if(vmem.vmem_table[i].size >= num_4K)//当整个4K页面的分区都被释放时，释放相应的页面
            {

                free_page(i);

            }

            return 0;

        }

    }

    if(vmem.count < MAX_FREE)//和前面后面的块都不连续且当前块数小于最大块数，直接插入块表
    {

        for(j = vmem.count; j > i; j--)
        {

            vmem.vmem_table[j] = vmem.vmem_table[j-1];

        }
        vmem.count++;

        if(vmem.count >= MAX_FREE)                                         //added by wang 2021.3.25
            udisp_str("malloc error: free partition table is full\n");

        vmem.vmem_table[i].vaddr = vaddr;	//插入

        vmem.vmem_table[i].size = size;

        if(size >= num_4K)  //当整个4K页面的分区都被释放时，释放相应的页面
        {

            free_page(i);

        }

        return 0;

    }

    return -1;

}


static int free_page(u32 index)
{

    u32 i,vaddr,size;       //size:added by wang 2021.4.6

    i = index;

    vaddr = vmem.vmem_table[index].vaddr;
    size = vmem.vmem_table[index].size;

    for(; i < vmem.count-1; i++)
    {

        vmem.vmem_table[i] = vmem.vmem_table[i+1];

    }

    vmem.count--;


    //edited by wang 2021.4.26
    if(vaddr + size == hp.heap_lin_limit)  //当要释放的逻辑页面在堆顶时，释放相应的物理页
    {

        u32 tmp_vaddr;


        for(tmp_vaddr=vaddr; tmp_vaddr < vaddr+size; tmp_vaddr+=num_4K)
        {

            free_ppage(tmp_vaddr);

        }
    }
    else
    {

        u32 tmp_vaddr;

        for(tmp_vaddr=vaddr; tmp_vaddr < vaddr+size; tmp_vaddr+=num_4K)
        {

            free_vpage(tmp_vaddr);

        }
    }

    /*
        else if(vaddr + num_4K == hp.heap_lin_limit)  //当要释放的逻辑页面在堆顶时，释放相应的物理页
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





static int free_ppage(u32 vaddr)  ////当要释放的逻辑页面在堆顶时，释放相应的物理页
{

    hp.heap_lin_limit -= num_4K;

    free_4k(vaddr);

    if(vplist.vpage_count>0)
    {

        while (vplist.vpage[vplist.vpage_count - 1] + num_4K == hp.heap_lin_limit) //循环更新堆指针limit,并释放堆顶页面

        {

            hp.heap_lin_limit -= num_4K;

            free_4k(vplist.vpage[vplist.vpage_count - 1]);

            vplist.vpage[vplist.vpage_count - 1] = 0;

            vplist.vpage_count--;

        }



    }

    return 0;

}

static int free_vpage(u32 vaddr) //First-Fit
{
    u32 i,j;

    for(i=0; i < vplist.vpage_count; i++)
    {

        if(vplist.vpage[i]>vaddr)  //First-Fit，方便循环释放物理页面

            break;

    }

    for(j=vplist.vpage_count; j>i; j--)
    {

        vplist.vpage[j] = vplist.vpage[j-1];

    }

    vplist.vpage[i]=vaddr;

    vplist.vpage_count++;

    if(vplist.vpage_count >= MAX_VPAGE)                          //added by wang 2021.3.25
        udisp_str("malloc error: vpage_list is full\n");


    return 0;
}


static u32 alloc_page()
{
    u32 vaddr;

    if(vplist.vpage_count > 0)  //如果空闲页面表非空，则直接从表中分配页面（这些页面已经映射到相应的物理页面）
    {

        vaddr = alloc_vpage();

    }

    else     //否则从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
    {

        vaddr = alloc_hpage();

    }

    return vaddr;
}

static u32 alloc_vpage()  //从空闲页面表中分配页面（这些页面已经映射到相应的物理页面）
{

    u32 i,vaddr;

    vaddr=vplist.vpage[0];

    vplist.vpage_count--;

    for(i = 0; i < vplist.vpage_count; i++)
    {

        vplist.vpage[i]=vplist.vpage[i+1];

    }

    return vaddr;

}

static u32 alloc_hpage()//从堆上分配虚拟页（堆指针limit+4k），同时要分配物理页面并进行映射
{

    u32 vaddr = hp.heap_lin_limit;

    hp.heap_lin_limit += num_4K;

    u32 vaddr_kernel=pmalloc_4K();           //分配4K物理页面
/*
    udisp_str("vaddr= ");
    udisp_int(vaddr);

    udisp_str("  vaddr_kernel= ");
    udisp_int(vaddr_kernel);
*/
    if(vaddr_kernel!=vaddr)
        udisp_str("malloc.c:error,vaddr is no consistent\n");

    if(hp.heap_lin_limit > HeapLinLimitMAX)
    {
        udisp_str("error:the heap is full\n");
        return 0;
    }
    else

        return vaddr;
}




static u32 pmalloc_4K()
{

    u32 vaddr=malloc_4k();

    return vaddr;
}



//test
void scan_heap_ptr()
{
    udisp_str("base=");
    udisp_int(hp.heap_lin_base);
    udisp_str("   limit=");
    udisp_int(hp.heap_lin_limit);
    udisp_str("\n");
}

void scan_page_list()
{
    u32 i;
    udisp_str("page_count=");
    udisp_int(vplist.vpage_count);
    udisp_str("\n");
    for(i=0; i<vplist.vpage_count; i++)
    {
        udisp_str("  addr=");
        udisp_int(vplist.vpage[i]);

    }
    udisp_str("\n");
}
void scan_vtable()
{
    u32 i;
    for(i = 0; i < vmem.count; i++)
    {
        udisp_int(i);
        udisp_str(":  addr=");
        udisp_int(vmem.vmem_table[i].vaddr);
        udisp_str("  size=");
        udisp_int(vmem.vmem_table[i].size);
        if(i%2==1)
            udisp_str("\n");
        else
            udisp_str("  ");
    }
    if(i%2==1)
        udisp_str("\n");
}

void scan_malloc_table()
{
    u32 i;
    for(i = 0; i < mal.count; i++)
    {
        udisp_int(i);
        udisp_str("  addr=");
        udisp_int(mal.mtable[i].vaddr);
        udisp_str("  size=");
        udisp_int(mal.mtable[i].size);
        if(i%2==1)
            udisp_str("\n");
        else
            udisp_str("  ");
    }
    if(i%2==1)
        udisp_str("\n");
}


/*

void test_malloc()
{
    //heap_init();

    umalloc(1024);//测试vmalloc中for循环外的if分支和pmalloc的else分支
    //即没有合适的空闲块分配4K页面并建立映射
    printf("\n-----------vtable-------\n");
    scan_vtable();

    printf("\n-----------malloc_table--------------\n");
    scan_malloc_table();

    umalloc(72);//测试vmalloc中for循环内的第一个if分支
    //pmalloc的大if分支及for循环内第一个if分支
    //即空闲表中有合适的分区，并且分区大小大于要分配的空间
    printf("\n-----------vtable-------\n");
    scan_vtable();

    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();

    umalloc(3000);//测试vmalloc中for循环内的第二个if分支
    //pmalloc的大if分支及for循环内if分支内的if分支
    //即空闲表中有合适的分区，并且分区大小等于要分配的空间
    //分配后要将该分区删除

    printf("\n-----------vtable-------\n");
    scan_vtable();

    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();

}

void test_alloc_free_page()
{
    int *p1,*p2,*p3,*p4,*p5,*p6;

    p1=umalloc(4000);  //测试alloc_hpage;

    p2=umalloc(4032);

    p3=umalloc(3960);

    p4=umalloc(4048);

    p5=umalloc(4080);

    scan_vtable();
    scan_heap_ptr();
    scan_page_list();

    printf("\n--------------------------------\n");
    ufree(p3);    //测试free_vpage
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    ufree(p4);    //测试free_vpage
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    ufree(p1);    //测试free_vpage
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    ufree(p2);    //测试free_vpage
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    p6=umalloc(3999); //测试alloc_vpage
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    ufree(p6);
    scan_heap_ptr();
    scan_page_list();

    printf("\n");
    ufree(p5);    //测试free_ppage
    scan_heap_ptr();
    scan_page_list();

}

void test_free()
{
    int vaddr1,vaddr2,vaddr3,vaddr4,vaddr5,vaddr6,vaddr7;
    //freemem[0].bgn_addr = 0x00400000;//4M
    //freemem[0].end_addr = 0x00600000;//6M
    //heap_init();  //初始化一个2M的页块

    scan_heap_ptr();
    vaddr1=umalloc(128);
    vaddr2=umalloc(2000);
    vaddr3=umalloc(512);

    scan_heap_ptr();
    vaddr4=umalloc(2048);
    vaddr5=umalloc(200);
    vaddr6=umalloc(1024);

    scan_heap_ptr();
    vaddr7=umalloc(3200);

    printf("\n-----------vtable-------\n");
    scan_vtable();


    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();


    ufree(vaddr1);//不属于同一页的分区即使连续也不合并
    printf("\n\nfree 128\n");

    printf("\n-----------vtable-------\n");
    scan_vtable();


    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();

    ufree(vaddr4);//前后都不连续，直接插入
    printf("\n\nfree 2048\n");

    printf("\n-----------vtable-------\n");
    scan_vtable();


    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();


    ufree(vaddr3);//仅和后面内存块连续，合并
    printf("\n\nfree 512\n");

    printf("\n-----------vtable-------\n");
    scan_vtable();


    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();

    ufree(vaddr5);//仅和前面内存块连续，合并
    printf("\n\nfree 200\n");

    printf("\n-----------vtable-------\n");
    scan_vtable();


    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();

    ufree(vaddr6);//和前面的内存块及后面内存块都连续，两次合并，合并后size=4K,释放页面
    printf("\n\nfree 1024\n");

    printf("\n-----------vtable-------\n");
    scan_vtable();

    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();
    printf("\n------------free_page----------------\n");
    scan_page_list();
    scan_heap_ptr();

    ufree(vaddr2);
    ufree(vaddr7);

    printf("\n-----------vtable-------\n");
    scan_vtable();

    printf("\n-----------malloc_table--------------\n\n");
    scan_malloc_table();
    scan_page_list();
    scan_heap_ptr();

}

*/


