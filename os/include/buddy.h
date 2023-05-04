/*************************************************************
*     buddy.h     add by wang   2021.3.3
**************************************************************/

#include "type.h"
#include "const.h"



#define FMIBuff		0x007ff000  //必须于load.inc保持一致, mingxuan 2021-8-25

#define MAX_ORDER 11        //页块链表数目
//#define BITMAP_SIZE 12288                          //memory can be 3G
#define BITMAP_SIZE 16384       //memory can be 4G
//#define buddy_size 16384      //bitmap二维数组第二维的大小，由buddy管理的内存大小决定,
                            //buddy_size = Max physic address/(4*1024*2*32)  when buddy_size=16384, memory can be 4G
#define LIST_SIZE 1024       //buddy每一个级别链表上最多有多少个空闲页块                  add by wang 2021.3.9
#define MAX_FREE_BLOCK 1024     //空闲物理内存分区的最大数目

#define WALL            0x10000000            //256M

#define KKWALL1		0x00400000            //4M
#define KUWALL1		0x00800000            //8M

#define KKWALL2         0x00800000            //8M
#define KUWALL2         0x02000000            //32M

struct free_area
{
    u32  free_list[LIST_SIZE];  //空闲页块线性表          edit by wang 2021.3.9
    u32  free_count;     //空闲页块数目
};

struct free_memory  //记录空闲物理内存分区的起始地址和终止地址，用于buddy的初始化
{
    u32 bgn_addr;
    u32 end_addr;
};


typedef struct buddy
{
    struct free_area free_area[MAX_ORDER]; //伙伴系统数组，每一个数组元素代表一种内存页块
    u32 total_mem_size;                    //added by wang 2021.8.26
    u32 current_mem_size;

}buddy;

typedef struct bmap
{
	u32 bitmap[MAX_ORDER][BITMAP_SIZE];
}bmap;


typedef struct free_mem  //空闲物理内存分区表，用于Buddy的初始化
{
    struct free_memory freemem[MAX_FREE_BLOCK];  //数组每一项记录一个可用物理内存分区的起始地址和终止地址
    u32 freemem_num;     //空闲物理内存分区数目
}free_mem;

u32 alloc_pages(buddy *bud,u32 order); //分配2^order页的页块
int free_pages(buddy *bud,u32 paddr,u32 order);
int memory_init(free_mem *free,buddy *bud);

extern buddy *kbud;
extern buddy *ubud;
extern u32 block_size[11];

extern u32 kernel_size;
extern int big_kernel;
extern u32 kernel_code_size;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
extern u32 test_phy_mem_size; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27
