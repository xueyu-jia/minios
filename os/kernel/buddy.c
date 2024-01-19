/*************************************************************
* 内存管理-buddy系统相关代码     add by wang   2021.3.3
**************************************************************/

#include "string.h"
#include "console.h"
#include "buddy.h"
#include "kmalloc.h"

struct page mem_map[ALL_PAGES];
buddy kbuddy, ubuddy;
buddy *kbud = &kbuddy;
buddy *ubud = &ubuddy;

u32 MemInfo[256] = {0}; //存放FMIBuff后1k内容
u32 block_size[MAX_ORDER];     //the block size of each order

int big_kernel = 0;        //当big_kernel=1时，表示大内核，big_kernel=0表示小内核，added by wang 2021.8.16
u32 kernel_size = 0;       //表示内核大小的全局变量，added by wang 2021.8.27
u32 kernel_code_size = 0;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
u32 test_phy_mem_size = 0; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27

PRIVATE void fragmentUse(u32 addr, u32 size);
PRIVATE u32 __find_buddy_pfn(u32 page_pfn, u32 order);
PRIVATE u32 page_is_buddy(u32 pfn, page *page, u32 order);
PRIVATE void del_page_from_free_list(page *page, buddy *bud,u32 order);
PRIVATE void add_to_free_list(page *page, buddy *bud,u32 order);
PRIVATE void set_page_order(page *page, u32 order);
PRIVATE void set_buddy_order(page *page, u32 order);
PRIVATE page* get_page_from_free_area(struct free_area *area);
PRIVATE void expand(buddy *bud, page *page, u32 low, u32 high);
void buddy_init(buddy *bud, u32 bgn_addr, u32 end_addr);

void memory_init()
{
    memcpy(MemInfo, (u32 *)FMIBuff, 1024); //复制内存

    test_phy_mem_size = MemInfo[MemInfo[0]];
    // int t, i, j;
	u32 i;
    // disp_str("memtest got memory available:\n");
    // disp_str("base_addr     end_addr\n");
    // for (t = 1; t <= MemInfo[0]; t++)
    // {
    //     disp_int(MemInfo[t]);
    //     disp_str("       ");
    //     if (t % 2 == 0)
    //         disp_str("\n");
    // }
    disp_str("test_phy_mem_size: ");
    disp_int(test_phy_mem_size);
    disp_str("\n");

    u32 bsize = num_4K;
    for (i = 0; i < MAX_ORDER; i++) {
        block_size[i] = bsize;
        bsize = 2 * bsize;
    }

    // TODO: ALL_PAGES 怎么确定
    u32 last_pfn = phy_to_pfn(test_phy_mem_size);
    for (i = 0; i < last_pfn; i++) {
        mem_map[i].inbuddy = TRUE;
    }
    for (; i < ALL_PAGES; i++) {
        mem_map[i].inbuddy = FALSE;
    }

    if (test_phy_mem_size < WALL) {
        big_kernel = 0;
        kernel_code_size = KKWALL1;
        kernel_size = SmallKernelSize;
        buddy_init(kbud, KKWALL1, KUWALL1);
        buddy_init(ubud, KUWALL1, test_phy_mem_size);
    }
    else {
        big_kernel = 1;
        kernel_code_size = KKWALL2;
        kernel_size = BigKernelSize;
        buddy_init(kbud, KKWALL2, KUWALL2);
        buddy_init(ubud, KUWALL2, test_phy_mem_size);
    }

    kmem_init(); //added by mingxuan 2021-3-8
    init_cache();
}

void buddy_init(buddy *bud, u32 bgn_addr, u32 end_addr) {
    
    bud->current_mem_size = 0;
    bud->total_mem_size = end_addr - bgn_addr;
    for (int i = 0; i < MAX_ORDER; i++) {
        bud->free_area[i].free_count = 0;
    }

    block_init(bgn_addr, end_addr, bud);
}

//added by wang 2021.6.8
int block_init(u32 bgn_addr, u32 end_addr, buddy *bud)
{
    int bsize, i;
    if (bgn_addr % num_4K != 0) {
        int tmp = bgn_addr % num_4K;
        if(bud==kbud) 
            fragmentUse(bgn_addr, num_4K - tmp); //edited by wang 2021.6.25
        bgn_addr = bgn_addr - tmp + num_4K;
    }

    bsize = end_addr - bgn_addr;
    while (bsize){
        for (i = MAX_ORDER-1; i >= 0; i--){
            if (bgn_addr % block_size[i] == 0 && bsize >= block_size[i]){
                free_pages(bud, pfn_to_page(phy_to_pfn(bgn_addr)), i);
                bgn_addr += block_size[i];
                bsize -= block_size[i];
                break;
            }
        }
        if (bsize < num_4K && bsize != 0){
            if(bud==kbud)                           //edited by qianglong 2021.4.19
                fragmentUse(bgn_addr, bsize); //edited by wang 2021.6.25
            break;
        }
    }

    return 0;
}

int free_pages(buddy *bud, page *page, u32 order) {
    u32 buddy_pfn;
	u32 combined_pfn;
	struct page *buddy;
    u32 pfn = page_to_pfn(page);
    u32 max_order = MAX_ORDER - 1;

continue_merging:
	while (order < max_order) {
		//查找伙伴块的页号
		buddy_pfn = __find_buddy_pfn(pfn, order);
		buddy = page + (buddy_pfn - pfn);
        //查找伙伴块是否在buddy系统中
		//若不在，继续执行done_merging
		if (!page_is_buddy(buddy_pfn, buddy, order))
			goto done_merging;
		//若在，将伙伴块从free_list中删除
		del_page_from_free_list(buddy, bud, order);
		//合并伙伴块
		combined_pfn = buddy_pfn & pfn;     // find head of block by & 
		page = page + (combined_pfn - pfn);
		pfn = combined_pfn;
		order++;
	}
done_merging:
	//设置当前page的order
	set_buddy_order(page, order);
	//将该page加入对应order的free_list中
	add_to_free_list(page, bud, order);

    bud->current_mem_size += block_size[order];
    return 0;
}

page* alloc_pages(buddy *bud, u32 order) {
	u32 current_order;
    page *page = NULL;
	struct free_area *area;
	
    //从经过计算所得要查找的order开始，到MAX_ORDER进行遍历，查找bud中每一个free_area[]中是否存在空闲块
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = &(bud->free_area[current_order]);
		page = get_page_from_free_area(area);
		//若未查到则继续查找
		if (!page)
			continue;
		//若查找到了，先将其从free_list中删除，如果需要再调用expand()函数进行大内存块拆分
		del_page_from_free_list(page, bud, current_order );
		expand(bud, page, order, current_order);

        if (page) {
            page->order = order;
            bud->current_mem_size -= block_size[order];
            return page;
        }
	}
	return NULL;
}

//added by wang 2021.6.25
PRIVATE void fragmentUse(u32 addr, u32 size) {
    malloced_insert(addr, size);
    kmem.total_mem_size += size;
    //do_kfree(addr);
    phy_kfree(addr); //modified by mingxuan 2021-8-17
}

PRIVATE u32 __find_buddy_pfn(u32 page_pfn, u32 order) {
	return page_pfn ^ (1 << order);
}

PRIVATE u32 page_is_buddy(u32 pfn, page *page, u32 order) {
	if (!page->inbuddy || page->order != order || pfn >= ALL_PAGES)
		return FALSE;
	return TRUE;
}

PRIVATE void del_page_from_free_list(page *page, buddy *bud,u32 order) {
    // list operation
    struct page *node = bud->free_area[order].free_list;
    if (node == page) {
        bud->free_area[order].free_list = page->next;
    } else {
        while (node->next != NULL) {
            if (node->next == page) {
                break;
            }
            node = node->next;
        }
        if (node->next != page) {
            disp_color_str("buddy: del_page_from_free_list, page not found\n", 0x74);
        }
        node->next = page->next;
    }
    // page -> next != NULL
    page->next = NULL ;
	page -> inbuddy = FALSE;
	bud->free_area[order].free_count--;
}

PRIVATE void add_to_free_list(page *page, buddy *bud,u32 order) {
	struct free_area *area = &bud->free_area[order];
	page->next = NULL;
	if (area->free_list==NULL) {
		
		area->free_list = page;
	} else {
		struct page * node;
		node = area->free_list;
		while (node->next) {
			node = node->next;
		}
		node->next = page;
	}
	area->free_count++;
}

PRIVATE void set_page_order(page *page, u32 order) {
	page->order = order;
}

PRIVATE void set_buddy_order(page *page, u32 order) {
	set_page_order(page, order);
	page->inbuddy = TRUE;
}

PRIVATE page* get_page_from_free_area(struct free_area *area) {   
    struct page * node ;
    //返回该链表第一个块或者返回NULL
    node = area->free_list; // *page
    return node;
}

PRIVATE void expand(buddy *bud, page *page, u32 low, u32 high) {
	u32 size = 1 << high;

	while (high > low) {
		high--;
		size >>= 1;
		add_to_free_list(&page[size], bud, high);
		set_buddy_order(&page[size], high);
	}
}

/*u32 alloc_pages(buddy *bud, u32 order) //分配2^order页的页块
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
}*/

//test buddy

void Scan_free_area(buddy *bud)
{
    u32 i, j;
    for (i = 0; i < MAX_ORDER; i++)
    {
        struct free_area *area = &bud->free_area[i];
        page *node = area->free_list;
        disp_str("   order = ");
        disp_int(i);
        disp_str("   nr_free=");
        disp_int(area->free_count);
        disp_str("  paddr:");

        while (node != NULL) {
            disp_int(pfn_to_phy(page_to_pfn(node)));
            disp_str("  ");
            node = node->next;
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
    page *first, *second, *third;

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
