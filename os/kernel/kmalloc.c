/*************************************************************
*内核地址空间小内存管理相关代码    add by wang  2021.3.3
**************************************************************/

#include <kernel/kmalloc.h>
#include <kernel/buddy.h>
#include <kernel/console.h>
//#include <kernel/../include/memman.h>    //deleted by mingxuan 2021-8-13

malloced mal;
free_kmem kmem;
//extern struct free_memory kfreemem[128];
//extern struct free_area kfree_area[Max_order];


// static void kfree_page(u32 index);
void malloced_insert(u32 addr, u32 size);
//static u32 do_kfree(u32 addr,u32 size);
//static u32 do_kfree_static(u32 addr,u32 size);  //modified by mingxuan 2021-3-25
// static u32 phy_kfree_static(u32 addr, u32 size); //modified by mingxuan 2021-8-17
static u32 which_order(u32 size);

//added by mingxuan 2021-3-8
void kmem_init()
{
    //kmem.kmem_table[0].addr=0x0;
    //kmem.kmem_table[0].size=4092;

    kmem.count = 0;
    mal.count = 0;
    kmem.total_mem_size = 0;
    kmem.current_mem_size = 0;
}

//added by wang 2021.6.8
u32 kmalloc_over4k(u32 size)
{
    u32 order = which_order(size);
    page *page = alloc_pages(kbud, order);
    u32 addr = pfn_to_phy(page_to_pfn(page));

    if (size < block_size[order]) //申请的内存小于order页块的大小，将剩余内存返回buddy系统，如申请10K，则分配一个16K页块，将剩余6K返回到buddy系统

        block_init(addr + size, addr + block_size[order], kbud); //通过初始化函数将剩余内存返还

    malloced_insert(addr, size);

    kmem.total_mem_size += size;
    if (addr == 0)
        disp_color_str("kmalloc_over4k:kmalloc Error,no memory", 0x74);

    return addr;
}

//added by wang 2021.6.8
u32 kfree_over4k(u32 addr)
{
    u32 i, size = 0;

    for (i = 0; i < mal.count; i++) //使用地址在分配表中查找已分配内存块
    {

        if (addr == mal.mtable[i].addr)
        {

            size = mal.mtable[i].size;

            break;
        }
    }
    if (i < mal.count)
    {

        mal.count--;

        for (; i < mal.count; i++)
        {

            mal.mtable[i] = mal.mtable[i + 1];
        }
    }
    else
    {

        disp_color_str("kfree_over4k error", 0x74);

        return -1; //查找失败
    }

    kmem.total_mem_size -= size;
    block_init(addr, addr + size, kbud);

    return 0;
}

u32 get_kmalloc_size(u32 addr)
{
    u32 i, size = 0;

    for (i = 0; i < mal.count; i++) //使用地址在分配表中查找已分配内存块
    {

        if (addr == mal.mtable[i].addr)
        {

            size = mal.mtable[i].size;

            break;
        }
    }
    return size;
}

//added by wang 2021.6.8
static u32 which_order(u32 size)
{

    u32 i;
    for (i = 0;; i++)
    {
        if (size <= block_size[i])
            break;
    }
    return i;
}

/*u32 kmalloc(u32 size) //核心态下为内核程序分配小内存
{
    u32 i, addr;

    if (size >= num_4K)
    {
        disp_color_str("_kmalloc: error:it doesn't support size over 4K \n",0x74);
        return 0;
    }
    for (i = 0; i < kmem.count; i++) //空闲分区表中有合适的分区，就直接分配
    {

        if (size <= kmem.kmem_table[i].size)
        {
            addr = kmem.kmem_table[i].addr;

            kmem.kmem_table[i].addr += size;

            kmem.kmem_table[i].size -= size;

            if (kmem.kmem_table[i].size == 0)
            {

                kmem.count--;

                for (; i < kmem.count; i++)
                {

                    kmem.kmem_table[i] = kmem.kmem_table[i + 1];
                }
            }
            malloced_insert(addr, size);

            kmem.current_mem_size -= size;
            return addr;
        }
    }
    if (i >= kmem.count) //否则分配一个4K的物理页面
    {

        //addr = do_kmalloc_4k();
        addr = phy_kmalloc_4k(); //modified by mingxuan 2021-8-16

        kmem.total_mem_size += num_4K;
        //do_kfree(addr + size,num_4K - size);  //将分配完剩余的内存块挂在空闲分区表上
        //do_kfree_static(addr + size,num_4K - size); //modified by mingxuan 2021-3-25
        phy_kfree_static(addr + size, num_4K - size); //modified by mingxuan 2021-8-17
    }

    malloced_insert(addr, size);

    return addr;
}*/

void malloced_insert(u32 addr, u32 size) //已经分配分区的记录表，供kfree使用
{
    u32 i = mal.count;

    mal.mtable[i].size = size;

    mal.mtable[i].addr = addr;

    mal.count++;

    if (mal.count >= MAX_MBLOCK) //add by wang 2021.3.25
        disp_color_str("kmalloc error:kmalloced table is full\n",0x74);
}

/*int kfree(u32 addr) //核心态下内核程序释放已申请的小内存
{

    u32 i, size;

    for (i = 0; i < mal.count; i++) //使用地址在分配表中查找已分配内存块
    {

        if (addr == mal.mtable[i].addr)
        {

            size = mal.mtable[i].size;

            break;
        }
    }
    if (i < mal.count)
    {

        mal.count--;

        for (; i < mal.count; i++)
        {

            mal.mtable[i] = mal.mtable[i + 1];
        }
    }
    else
    {

        disp_color_str("kfree error", 0x74);

        return -1; //查找失败
    }

    //do_kfree(addr,size);           //释放该内存块
    //do_kfree_static(addr,size);     //modified by mingxuan 2021-3-25
    phy_kfree_static(addr, size); //modified by mingxuan 2021-8-17

    return 0;
}

static void kfree_page(u32 index)
{
    u32 i;

    i = index;

    u32 paddr = kmem.kmem_table[index].addr;

    kmem.total_mem_size -= num_4K;

    kmem.current_mem_size -= num_4K;
    //do_kfree_4k(paddr);
    phy_kfree_4k(paddr); //modified by mingxuan 2021-8-16

    for (; i < kmem.count - 1; i++)
    {

        kmem.kmem_table[i] = kmem.kmem_table[i + 1];
    }

    kmem.count--;
}*/

//static u32 do_kfree(u32 addr,u32 size)
/*
static u32 do_kfree_static(u32 addr,u32 size)   //modified by mingxuan 2021-3-25
{
    //释放
    int i,j,pageid;

    pageid = addr / num_4K;

    for(i = 0; i < kmem.count; i++)
    {

        if(kmem.kmem_table[i].addr > addr)//FirstFit
        {

            break;

        }
    }
    if(i > 0)//要释放的块的地址不是所有块中最低的，可能和前一块合并
    {
		//如果前一块和当前块连续，并且同属一页，则合并
        if(kmem.kmem_table[i-1].addr + kmem.kmem_table[i-1].size == addr && kmem.kmem_table[i-1].addr / num_4K == pageid)
        {

            kmem.kmem_table[i-1].size += size;

            if(i < kmem.count)//第i-1块非最后一块，则可能和后一块合并
            {
				//和前面的块连续的同时还和后一块连续并且同属一页则合并
                if(addr + size == kmem.kmem_table[i].addr && kmem.kmem_table[i].addr / num_4K == pageid)
                {

                    kmem.kmem_table[i-1].size += kmem.kmem_table[i].size;

                    kmem.count--;

                    j = i;

                    for(; j < kmem.count; j++)
                    {

                        kmem.kmem_table[j] = kmem.kmem_table[j+1];

                    }

                }
            }

            if(kmem.kmem_table[i-1].size == num_4K)
            {

                kfree_page(i-1);

            }

            return 0;

        }

    }
    if(i < kmem.count)//i=0以及和前面的分区不连续都走这个分支，即和前面的块不连续或者前面无块，判断和后面的块是否连续
    {

        if(addr + size == kmem.kmem_table[i].addr && kmem.kmem_table[i].addr/num_4K == pageid)
        {

            kmem.kmem_table[i].addr = addr;

            kmem.kmem_table[i].size += size;

            if(kmem.kmem_table[i].size == num_4K)
            {

                kfree_page(i);

            }

            return 0;

        }

    }

    if(kmem.count < MAX_KFREE)//没有可以合并的，直接插入
    {
        for(j = kmem.count; j > i; j--)
        {

            kmem.kmem_table[j] = kmem.kmem_table[j-1];

        }

        kmem.count++;

        if(kmem.count >= MAX_KFREE)                   //add by wang 2021.3.25
            disp_str("kmalloc error: free partition table is full");


        kmem.kmem_table[i].addr = addr;

        kmem.kmem_table[i].size = size;

        if(size == num_4K)
        {

            kfree_page(i);

        }

        return 0;

    }

    return -1;
}
*/
/*static u32 phy_kfree_static(u32 addr, u32 size) //modified by mingxuan 2021-8-17
{
    //释放
    int i, j, pageid;

    pageid = addr / num_4K;

    for (i = 0; i < kmem.count; i++)
    {

        if (kmem.kmem_table[i].addr > addr) //FirstFit
        {

            break;
        }
    }
    if (i > 0) //要释放的块的地址不是所有块中最低的，可能和前一块合并
    {
        //如果前一块和当前块连续，并且同属一页，则合并
        if (kmem.kmem_table[i - 1].addr + kmem.kmem_table[i - 1].size == addr && kmem.kmem_table[i - 1].addr / num_4K == pageid)
        {

            kmem.kmem_table[i - 1].size += size;

            if (i < kmem.count) //第i-1块非最后一块，则可能和后一块合并
            {
                //和前面的块连续的同时还和后一块连续并且同属一页则合并
                if (addr + size == kmem.kmem_table[i].addr && kmem.kmem_table[i].addr / num_4K == pageid)
                {

                    kmem.kmem_table[i - 1].size += kmem.kmem_table[i].size;

                    kmem.count--;

                    j = i;

                    for (; j < kmem.count; j++)
                    {

                        kmem.kmem_table[j] = kmem.kmem_table[j + 1];
                    }
                }
            }

            if (kmem.kmem_table[i - 1].size == num_4K)
            {

                kfree_page(i - 1);
            }

            kmem.current_mem_size += size;
            return 0;
        }
    }
    if (i < kmem.count) //i=0以及和前面的分区不连续都走这个分支，即和前面的块不连续或者前面无块，判断和后面的块是否连续
    {

        if (addr + size == kmem.kmem_table[i].addr && kmem.kmem_table[i].addr / num_4K == pageid)
        {

            kmem.kmem_table[i].addr = addr;

            kmem.kmem_table[i].size += size;

            if (kmem.kmem_table[i].size == num_4K)
            {

                kfree_page(i);
            }

            kmem.current_mem_size += size;
            return 0;
        }
    }

    if (kmem.count < MAX_KFREE) //没有可以合并的，直接插入
    {
        for (j = kmem.count; j > i; j--)
        {

            kmem.kmem_table[j] = kmem.kmem_table[j - 1];
        }

        kmem.count++;

        if (kmem.count >= MAX_KFREE) //add by wang 2021.3.25
        {
            disp_str("kmalloc error: free partition table is full");
            return -1;
        }

        kmem.kmem_table[i].addr = addr;

        kmem.kmem_table[i].size = size;

        if (size == num_4K)
        {

            kfree_page(i);
        }

        kmem.current_mem_size += size;
        return 0;
    }

    return -1;
}*/

//test
// #define KMALLOC_TEST
#ifdef KMALLOC_TEST
void scan_ktable()
{
    u32 i;
    for (i = 0; i < kmem.count; i++)
    {
        u32 pageid = kmem.kmem_table[i].addr / num_4K;
        disp_int(i);
        //        disp_str(":pageid=");
        //        disp_int(pageid);
        disp_str(":  addr=");
        disp_int(kmem.kmem_table[i].addr);
        disp_str("  size=");
        disp_int(kmem.kmem_table[i].size);
        if (i % 2 == 1)
            disp_str("\n");
        else
            disp_str("  ");
    }
    if (i % 2 == 1)
        disp_str("\n");
}

void scan_malloc_table()
{
    u32 i;
    for (i = 0; i < mal.count; i++)
    {

        disp_int(i);
        disp_str(": addr=");
        disp_int(mal.mtable[i].addr);
        disp_str("   size=");
        disp_int(mal.mtable[i].size);
        if (i % 2 == 1)
            disp_str("\n");
        else
            disp_str("  ");
    }
    if (i % 2 == 1)
        disp_str("\n");
}

void scan_mem_size()
{
    disp_str("kbud->total_mem_size=");
    disp_int(kbud->total_mem_size);
    disp_str("    kbud->current_mem_size=");
    disp_int(kbud->current_mem_size);
    disp_str("\n");
    disp_str("kmem.total_mem_size=");
    disp_int(kmem.total_mem_size);
    disp_str("    kmem.current_mem_size=");
    disp_int(kmem.current_mem_size);
    disp_str("\ntotal_mem_size=");
    disp_int(ubud->current_mem_size + kbud->current_mem_size + kmem.current_mem_size);
    disp_str("\n");
}

void test_kmalloc_kfree_over4k()
{
    //int vaddr1,vaddr2,vaddr3;
    int paddr1, paddr2, paddr3;

    disp_str("-----------initial buddy----------\n");

    Scan_free_area(kbud);

    disp_str("-----------initial ktable-------\n");

    scan_ktable();

    disp_str("-----------initial malloc_table--------------\n");

    scan_malloc_table();

    //vaddr1=do_kmalloc(10240);//测试_kmalloc中for循环外的if分支
    paddr1 = phy_kmalloc(10240); //modified by mingxuan 2021-8-16
                                 /*                              //即没有合适的空闲块分配4K页面并建立映射

    scan_mem_size();
    disp_str("-----------buddy----------\n");

    Scan_free_area(kbud);

    disp_str("-----------ktable-------\n");

    scan_ktable();

    disp_str("-----------malloc_table--------------\n");

    scan_malloc_table();
*/

    paddr2 = phy_kmalloc(1024);
    scan_mem_size();

    paddr3 = phy_kmalloc(20480);

    scan_mem_size();

    /*
    disp_str("-----------buddy----------\n");

    Scan_free_area(kbud);

    disp_str("-----------ktable-------\n");

    scan_ktable();

    disp_str("-----------malloc_table--------------\n");

    scan_malloc_table();
*/

    /*
    do_kfree(vaddr1);
    do_kfree(vaddr2);
    do_kfree(vaddr3);
    */
    phy_kfree(paddr1);
    phy_kfree(paddr2);
    phy_kfree(paddr3);

    disp_str("-----------buddy----------\n");

    Scan_free_area(kbud);

    disp_str("-----------ktable-------\n");

    scan_ktable();

    disp_str("-----------malloc_table--------------\n");

    scan_malloc_table();
}

void test_kmalloc()
{
    //int vaddr1,vaddr2,vaddr3;
    int paddr1, paddr2, paddr3; //modified by mingxuan 2021-8-16

    //    disp_str("-----------initial buddy----------\n");

    //    Scan_free_area(kbud);
    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();

    disp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    //vaddr1=do_kmalloc(1024);//测试_kmalloc中for循环外的if分支
    //即没有合适的空闲块分配4K页面并建立映射
    paddr1 = kern_kmalloc(1024);

    //    Scan_free_area(kbud);
    //    disp_str("-----------ktable-------\n");

    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");

    //    scan_malloc_table();

    //vaddr2=do_kmalloc(72);//测试kmalloc中for循环内的第一个if分支
    //即空闲表中有合适的分区，并且分区大小大于要分配的空间
    paddr2 = kern_kmalloc(72);

    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();
    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();

    //vaddr3=do_kmalloc(3000);//测试kmalloc中for循环内的第二个if分支
    //即空闲表中有合适的分区，并且分区大小等于要分配的空间
    //分配后要将该分区删除
    paddr3 = kern_kmalloc(3000);

    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();
}

void test_kfree()
{
    //int vaddr1,vaddr2,vaddr3,vaddr4,vaddr5,vaddr6,vaddr7;
    int paddr1, paddr2, paddr3, paddr4, paddr5, paddr6, paddr7;

    //    disp_str("-----------buddy initial----------\n");
    //    Scan_free_area(kbud);
    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();
    paddr1 = kern_kmalloc(1024);
    scan_mem_size();
    paddr2 = kern_kmalloc(2000);
    scan_mem_size();
    paddr3 = kern_kmalloc(512);
    scan_mem_size();

    paddr4 = kern_kmalloc(2048);
    scan_mem_size();
    paddr5 = kern_kmalloc(1000);
    scan_mem_size();
    paddr6 = kern_kmalloc(1024);
    scan_mem_size();

    paddr7 = kern_kmalloc(3200);
    scan_mem_size();
    //    disp_str("after kmalloc:----------buddy free_area----------\n");
    //    Scan_free_area(kbud);
    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();

    phy_kfree(paddr1); //不属于同一页的分区即使连续也不合并,和第二页的剩余分区地址连续
    scan_mem_size();
    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();
    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();

    phy_kfree(paddr2); //仅和前面内存块连续，合并
    scan_mem_size();

    disp_str("-----------ktable-------\n");
    scan_ktable();

    disp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    phy_kfree(paddr6); //仅和后面内存块连续，合并
    scan_mem_size();

    disp_str("-----------ktable-------\n");
    scan_ktable();

    disp_str("-----------malloc_table------------\n");
    scan_malloc_table();

    phy_kfree(paddr4); //和前后分区都不连续，直接插入

    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();
    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();

    phy_kfree(paddr3); //和前面的内存块及后面内存块都连续，两次合并，合并后size=4K,释放页面
    //    disp_str("----------buddy free_area----------\n");
    //    Scan_free_area(kbud);
    //    disp_str("-----------ktable-------\n");
    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");
    //    scan_malloc_table();

    //    do_kfree(vaddr5);
    //    do_kfree(vaddr7);
    //    disp_str("-------free_area after kmalloc/kfree--------\n");
    //    Scan_free_area(kbud);
    //   disp_str("-----------ktable-------\n");

    //    scan_ktable();

    //    disp_str("-----------malloc_table--------------\n");

    //    scan_malloc_table();

    /*
    vaddr1=do_kmalloc(1200);

    disp_str("-------free_area after kmalloc/kfree--------\n");
    Scan_free_area(kbud);
    disp_str("-----------ktable-------\n");

    scan_ktable();

    disp_str("-----------malloc_table--------------\n");

    scan_malloc_table();
*/
}
#endif
