/*************************************************************
* 内存管理-buddy系统相关代码     add by wang   2021.3.3
**************************************************************/

/*
#include "buddy.h"
#include "type.h"
#include "memman.h"
*/
#include "../include/buddy.h"
#include "../include/type.h"
#include "../include/kmalloc.h"
//#include "../include/memman.h"    //deleted by mingxuan 2021-8-13

//struct free_area free_area[MAX_ORDER]; //伙伴系统数组，每一个数组元素代表一种内存页块
//u32 bitmap[11][128];    //用来标识伙伴的状态，0,1,2...11代表order
//struct free_memory freemem[128];

//int memory_init(buddy *bud); //初始化内存
//u32 alloc_pages(buddy *bud,u32 order); //分配2^order个4K页面
//int free_pages(buddy *bud,u32 paddr,u32 order); //释放页面

buddy kbuddy, ubuddy;
buddy *kbud = &kbuddy;
buddy *ubud = &ubuddy;
bmap bmap1;
bmap *bp = &bmap1;
free_mem kfreemem, ufreemem;
free_mem *kfreem = &kfreemem;
free_mem *ufreem = &ufreemem;

u32 MemInfo[256] = {0}; //存放FMIBuff后1k内容
u32 block_size[11];     //the block size of each order

int big_kernel = 0;        //当big_kernel=1时，表示大内核，big_kernel=0表示小内核，added by wang 2021.8.16
u32 kernel_size = 0;       //表示内核大小的全局变量，added by wang 2021.8.27
u32 kernel_code_size = 0;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
u32 test_phy_mem_size = 0; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27

static u32 Split_buddy(buddy *bud, u32 order, u32 tmp);
static int list_insert(buddy *bud, u32 order, u32 node);
static int merge_buddy(buddy *bud, u32 freepage, u32 index, u32 order);
static u32 intpow(int ji, int zhi);
static void fragmentUse(u32 addr, u32 size);

void buddy_init()
{

    memcpy(MemInfo, (u32 *)FMIBuff, 1024); //复制内存

    test_phy_mem_size = MemInfo[MemInfo[0]];
    int t, i, j;
    disp_str("memtest got memory available:\n");
    disp_str("base_addr     end_addr\n");
    for (t = 1; t <= MemInfo[0]; t++)
    {
        disp_int(MemInfo[t]);
        disp_str("       ");
        if (t % 2 == 0)
            disp_str("\n");
    }
    disp_str("test_phy_mem_size: ");
    disp_int(test_phy_mem_size);
    disp_str("\n");

    u32 bsize = num_4K;

    for (i = 0; i < 11; i++)
    {
        block_size[i] = bsize;
        bsize = 2 * bsize;
    }

    for (i = 0; i < MAX_ORDER; i++)
    {

        for (j = 0; j < BITMAP_SIZE; j++)
        {

            bp->bitmap[i][j] = 0;
        }
    }

    if (test_phy_mem_size < WALL)
    {
        big_kernel = 0;

        kernel_code_size = KKWALL1;

        kernel_size = SmallKernelSize;

        kbuddy_init(KKWALL1, KUWALL1);

        ubuddy_init(KUWALL1, test_phy_mem_size);
    }
    else
    {
        big_kernel = 1;

        kernel_code_size = KKWALL2;

        kernel_size = BigKernelSize;

        kbuddy_init(KKWALL2, KUWALL2);

        ubuddy_init(KUWALL2, test_phy_mem_size);
    }

    kmem_init(); //added by mingxuan 2021-3-8
}

void ubuddy_init(u32 bgn_addr, u32 end_addr)
{
    int i;

    ufreem->freemem[0].bgn_addr = bgn_addr;

    ufreem->freemem[0].end_addr = end_addr;

    ufreem->freemem_num = 1;

    ubud->current_mem_size = 0;

    ubud->total_mem_size = end_addr - bgn_addr;
    for (i = 0; i < MAX_ORDER; i++)
    {

        ubud->free_area[i].free_count = 0;
    }

    memory_init(ufreem, ubud);
}

void kbuddy_init(u32 bgn_addr, u32 end_addr)
{
    int i;

    kfreem->freemem[0].bgn_addr = bgn_addr;

    kfreem->freemem[0].end_addr = end_addr;

    kfreem->freemem_num = 1;

    kbud->current_mem_size = 0;

    kbud->total_mem_size = end_addr - bgn_addr;
    for (i = 0; i < MAX_ORDER; i++)
    {

        kbud->free_area[i].free_count = 0;
    }

    memory_init(kfreem, kbud);
}

//edit by wang 2021.6.7
int memory_init(free_mem *free, buddy *bud) //将空闲内存挂在buddy系统上
{
    u32 i, bgn_addr, end_addr;

    for (i = 0; i < free->freemem_num; i++)
    {

        bgn_addr = free->freemem[i].bgn_addr;

        end_addr = free->freemem[i].end_addr;

        block_init(bgn_addr, end_addr, bud);
    }

    return 0;
}

//added by wang 2021.6.25
static void fragmentUse(u32 addr, u32 size)
{
    malloced_insert(addr, size);
    kmem.total_mem_size += size;
    //do_kfree(addr);
    phy_kfree(addr); //modified by mingxuan 2021-8-17
}

//added by wang 2021.6.8
int block_init(u32 bgn_addr, u32 end_addr, buddy *bud)
{

    int bsize, i;

    if (bgn_addr % num_4K != 0)

    {

        int tmp = bgn_addr % num_4K;

        fragmentUse(bgn_addr, num_4K - tmp); //edited by wang 2021.6.25

        bgn_addr = bgn_addr - tmp + num_4K;
    }

    bsize = end_addr - bgn_addr;

    while (bsize)

    {
        for (i = 10; i >= 0; i--)
        {
            if (bgn_addr % block_size[i] == 0 && bsize >= block_size[i])
            {
                free_pages(bud, bgn_addr, i);
                bgn_addr += block_size[i];
                bsize -= block_size[i];
                break;
            }
        }
        if (bsize < num_4K && bsize != 0)
        {
            fragmentUse(bgn_addr, bsize); //edited by wang 2021.6.25
            break;
        }
    }

    return 0;
}

u32 alloc_pages(buddy *bud, u32 order) //分配2^order页的页块
{

    u32 paddr;

    if (bud->free_area[order].free_count > 0) //链表非空，直接取下一个页块
    {

        u32 i = 0;

        paddr = bud->free_area[order].free_list[0];

        bud->free_area[order].free_count--;

        for (; i < bud->free_area[order].free_count; i++)

            bud->free_area[order].free_list[i] = bud->free_area[order].free_list[i + 1];

        u32 index, bit, arr_index, pageid;

        pageid = paddr / num_4K;

        index = pageid >> (order + 1); //设置map位，用于回收

        //bit = (index+1) % 32;
        if ((index + 1) % 32 == 0)
            bit = 32;

        else

            bit = (index + 1) % 32;

        arr_index = index / 32;

        bp->bitmap[order][arr_index] ^= intpow(2, 32 - bit);
    }
    else //当前order页块链表为空
    {

        u32 tmp = order;

        while (bud->free_area[tmp].free_count == 0) //向上搜寻非空的链表
        {

            tmp++;

            if (tmp >= MAX_ORDER)
            {

                disp_color_str("buddy:alloc_pages Error,no available memory in the buddy\n", 0x74);

                return 0; //无可用内存
            }
        }

        paddr = Split_buddy(bud, order, tmp); //在上级链表中找到未分配的页块，将其拆分
    }

    bud->current_mem_size -= block_size[order];
    return paddr;
}

int free_pages(buddy *bud, u32 paddr, u32 order) //释放2^order页的页块
{

    u32 index, bit, arr_index, tag, pageid, order1;

    if (paddr % num_4K != 0)
    {
        disp_color_str("buddy:free_pages:Address error!", 0x74);
        return -1;
    }

    order1 = order;

    pageid = paddr / num_4K;

    index = pageid >> (1 + order);

    if ((index + 1) % 32 == 0)

        bit = 32;
    else

        bit = (index + 1) % 32;

    arr_index = index / 32;

    tag = bp->bitmap[order][arr_index] & intpow(2, 32 - bit);

    if (tag == 0 || order == 10) //如果伙伴页块被分配，则直接将本页块挂在链表上
    {

        list_insert(bud, order, paddr);

        //map[order][index] = map[order][index]^1;

        bp->bitmap[order][arr_index] ^= intpow(2, 32 - bit);

        bud->free_area[order].free_count++;

        if (bud->free_area[order].free_count >= LIST_SIZE) //add by wang 2021.3.25
        {
            disp_color_str("buddy error: order", 0x74);
            disp_int(order);
            disp_color_str("liner list is full\n", 0x74);
            return -1;
        }
    }

    else //如果伙伴页块空闲，则要合并挂入order+1链表
    {

        u32 newfree;

        while (tag != 0 && order < 10) //当order=10时不再合并
        {

            paddr = merge_buddy(bud, paddr, index, order);

            order++;

            index = paddr / num_4K >> (order + 1);

            //bit = (index+1) % 32;
            if ((index + 1) % 32 == 0)

                bit = 32;
            else

                bit = (index + 1) % 32;

            arr_index = index / 32;

            tag = bp->bitmap[order][arr_index] & intpow(2, 32 - bit);
        }

        newfree = paddr;

        list_insert(bud, order, newfree);

        //map[order][index] = map[order][index]^1;

        bp->bitmap[order][arr_index] ^= intpow(2, 32 - bit);

        bud->free_area[order].free_count++;

        if (bud->free_area[order].free_count >= LIST_SIZE) //add by wang 2021.3.25
        {
            disp_color_str("buddy error: order", 0x74);
            disp_int(order);
            disp_color_str("liner list is full\n", 0x74);
            return -1;
        }
    }

    bud->current_mem_size += block_size[order1];

    return 0;
}

static u32 intpow(int ji, int zhi)
{
    if (zhi == 0)

        return 1;

    else

        return ji * intpow(ji, zhi - 1);
}

//将高一级的页块分解成两个低一级的buddy,order标识要分配页面的级别，tmp表示找到的非空链表的级别
static u32 Split_buddy(buddy *bud, u32 order, u32 tmp)
{
    //从free_area[tmp]上取下一个页块
    u32 paddr, count, i = 0;

    count = bud->free_area[tmp].free_count;

    paddr = bud->free_area[tmp].free_list[i];

    for (; i < count; i++)

        bud->free_area[tmp].free_list[i] = bud->free_area[tmp].free_list[i + 1];

    bud->free_area[tmp].free_count--;

    u32 index, bit, arr_index, pageid;

    pageid = paddr / num_4K;

    index = pageid >> (tmp + 1); //设置map位，用于回收

    //bit = (index+1) % 32;
    if ((index + 1) % 32 == 0)

        bit = 32;

    else

        bit = (index + 1) % 32;

    arr_index = index / 32;

    bp->bitmap[tmp][arr_index] ^= (u32)intpow(2, 32 - bit);

    while (tmp > order) //循环分解页块
    {

        u32 newnode;

        tmp--;

        //newnode = (list_node*)malloc(sizeof(list_node));
        newnode = paddr + (intpow(2, tmp)) * num_4K;

        list_insert(bud, tmp, paddr);

        bud->free_area[tmp].free_count++;

        if (bud->free_area[order].free_count >= LIST_SIZE) //add by wang 2021.3.25
        {
            disp_color_str("buddy error: order", 0x74);
            disp_int(order);
            disp_color_str("liner list is full\n", 0x74);
        }

        index = paddr / num_4K >> (tmp + 1); //设置map位，用于回收

        if ((index + 1) % 32 == 0)

            bit = 32;

        else

            bit = (index + 1) % 32;

        arr_index = index / 32;

        bp->bitmap[tmp][arr_index] ^= intpow(2, 32 - bit);

        paddr = newnode;
    }

    return paddr;
}

//向free_area[order]链表上插入一个页块
static int list_insert(buddy *bud, u32 order, u32 paddr)
{

    u32 i, j, count;

    count = bud->free_area[order].free_count;

    for (i = 0; i < count; i++)
    {

        if (bud->free_area[order].free_list[i] > paddr)

            break;
    }

    for (j = count; j > i; j--)
    {

        bud->free_area[order].free_list[j] = bud->free_area[order].free_list[j - 1];
    }

    bud->free_area[order].free_list[i] = paddr;

    return 0;
}

static int merge_buddy(buddy *bud, u32 freepage, u32 index, u32 order) //合并buddy
{

    u32 i, j;

    for (i = 0; i < bud->free_area[order].free_count; i++)
    {

        if ((bud->free_area[order].free_list[i] / num_4K >> (order + 1)) == index) //找到伙伴页块
        {

            if (bud->free_area[order].free_list[i] < freepage)
            {

                freepage = bud->free_area[order].free_list[i];
            }

            for (j = i; j < bud->free_area[order].free_count; j++)
            {

                bud->free_area[order].free_list[j] = bud->free_area[order].free_list[j + 1];
            }

            break;
        }
    }

    if (i >= bud->free_area[order].free_count)
    {

        return 0;
    }
    else
    {

        bud->free_area[order].free_count--;

        index = freepage / num_4K >> (order + 1);

        int bit, arr_index;

        if ((index + 1) % 32 == 0)

            bit = 32;

        else

            bit = (index + 1) % 32;

        arr_index = index / 32;

        bp->bitmap[order][arr_index] ^= intpow(2, 32 - bit);

        return freepage;
    }
}

//test buddy

void Scan_free_area(buddy *bud)
{
    u32 i, j;
    for (i = 0; i < MAX_ORDER; i++)
    {

        disp_str("   order = ");
        disp_int(i);
        disp_str("   nr_free=");
        disp_int(bud->free_area[i].free_count);
        disp_str("  paddr:");

        for (j = 0; j < bud->free_area[i].free_count; j++)
        {
            disp_int(bud->free_area[i].free_list[j]);
            disp_str("  ");
        }
        disp_str("\n");
    }
}

void test_kbud_mem_size()
{
    u32 i;
    for (i = 0; i < 10; i++)
    {
        disp_str("before:kbud_mem_size=");
        disp_int(kbud->current_mem_size);
        u32 addr = phy_kmalloc_4k();
        phy_kfree_4k(addr);
        disp_str("after:kbud_mem_size=");
        disp_int(kbud->current_mem_size);
    }
}

void test_alloc_pages()
{

    disp_str("-------------initial state-----------\n");
    Scan_free_area(kbud);

    disp_str("\n-------------test1----------------\n");
    disp_str("alloc_pages(kbud,5)\n");
    alloc_pages(kbud, 5); //测试链表非空
    Scan_free_area(kbud);

    //    disp_str("\n-------------test2----------------\n");
    //    disp_str("alloc_pages(kbud,4)\n");
    alloc_pages(kbud, 4); //测试order链表为空，上级链表非空
                          //    Scan_free_area(kbud);

    disp_str("\n-------------test3----------------\n");
    disp_str("alloc_pages(kbud,10)\n");
    alloc_pages(kbud, 10); //no enough memory;
    Scan_free_area(kbud);
}

void test_free_pages()
{
    u32 first, second, third;

    //    disp_str("\n-----------------before alloc------------\n");
    //    Scan_free_area(ubud);

    first = alloc_pages(ubud, 1);

    second = alloc_pages(ubud, 1);

    third = alloc_pages(ubud, 10);

    //    disp_str("\n--------------initial state-------------\n");
    //    Scan_free_area(ubud);

    free_pages(ubud, first, 1); //测试if(tag==0),伙伴被分配直接挂在链表上
                                //    disp_str("\n------------test1----------\n");
                                //    disp_str("free_pages(ubud,first,1)\n");
                                //    Scan_free_area(ubud);

    free_pages(ubud, second, 1); //测试else 伙伴未被分配，合并挂在order+1链表
                                 //    disp_str("\n------------test2----------\n");
                                 //    disp_str("free_pages(ubud,second,1)\n");
                                 //    Scan_free_area(ubud);

    free_pages(ubud, third, 10); //测试if(order==10)，直接挂在链表上。
                                 //    disp_str("\n------------test3----------\n");
                                 //    disp_str("free_pages(ubud,third,10)\n");
                                 //    Scan_free_area(ubud);

    first = alloc_pages(ubud, 9);
    second = alloc_pages(ubud, 9);
    //    Scan_free_area(ubud);

    free_pages(ubud, first, 9);
    free_pages(ubud, second, 9); //merge to order=10 then stop,no longer merge
    disp_str("\n------------test4----------\n");
    Scan_free_area(ubud);
}
