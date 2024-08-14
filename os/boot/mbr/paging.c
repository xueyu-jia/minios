//ported by sundong 2023.3.26
#include <mbr/loaderprint.h>
#include <mbr/memblock.h>
#include <mbr/paging.h>
#include <mbr/string.h>
#include <mbr/x86.h>
static phyaddr_t phy_malloc_pde =
    PageDirBase;  // 读汇编得到页目录存放地址宏定义
static phyaddr_t phy_malloc_pte =
    PageTblBase;  // 读汇编得到页表存放地值的宏定义,修改名称

static phyaddr_t phy_kmalloc_base = PhyMallocBase;
void paging(u32 MemChkBuf,u32 MCRNumber) {
  lprintf("----start paging---- \n");
  u32 size = get_mem_size(MemChkBuf, MCRNumber);
  size = size>_32M?_32M:size;
  mem_check_32M();
  lprintf("memsize %d \n", size);
  u32 cr3 = PageDirBase;
  memset((void*)PageDirBase,0,PGSIZE);
  map(cr3, size);
  lcr3(PageDirBase);  // load cr3
  u32 flag;
  flag = rcr0();
  flag =flag | 0x80000000;  // CR0的位31是分页（Paging）标志。当设置该位时即开启了分页机制
  lcr0(flag);
  lprintf("----finish paging----\n");
  return;
}

// 根据汇编中得到的ADR结构体数量及ADR结构体的内容存放的起始地址，计算内存大小
u32 get_mem_size(u32 MemChkBuf, u32 MCRNumber) {
  u32 size = 0;
  struct ARDStruct *adr =
      (void *)MemChkBuf;  // 第一个结构体的起始指针指向MemChkBuf
  for (unsigned int cnt = 0; cnt < MCRNumber; adr++)  // 每次循环读取一个结构体的内容
  {
    cnt++;
    print_mem(adr);
    if (adr->dwType == 1)  // 当结构体的成员变量dwType为1的时候，代表为OS可用
    {
      if (adr->dwBaseAddrLow + adr->dwLengthLow >
          size)  // 根据结构体的基地址的低32位以及长度(字节)的低32位计算内存大小
      {
        size = adr->dwBaseAddrLow +
               adr->dwLengthLow;  // 根据读取的结构体的内容计算内存大小
      }
    }
  }
  return size;
}
// 分配一页页表
phyaddr_t pte_malloc_4k(void) {
  phyaddr_t paddr = phy_malloc_pte;
  memset((void*)phy_malloc_pte,0,PGSIZE);
  phy_malloc_pte += PGSIZE;
  return paddr;
}

phyaddr_t phy_kmalloc(u32 size){
  memset((void*)phy_malloc_pte,0,PGSIZE);
  phy_kmalloc_base+=size;
  return phy_kmalloc_base-size;

}
int row = 0;
void lin_mapping_phy(u32 cr3,          // 页目录起始
                            uintptr_t laddr,  // 虚拟地址
                            phyaddr_t paddr,  // 物理地址
                            u32 pte_flag)     // 权限
{
  // 分配PTE
  uintptr_t *pde_ptr = (uintptr_t *)cr3;
  if ((pde_ptr[PDX(laddr)] & PG_P) == 0) {  // 若虚拟地址计算出的页目录项没有东西，则分配
    phyaddr_t pte_phy = pte_malloc_4k();  // 分配
   // memset((void *)pte_phy, 0, PGSIZE);   // 分配的内存块先全部填充0
    pde_ptr[PDX(laddr)] = pte_phy | PG_P | PG_USU |
                          PG_RWW;  // 将分配的地址写回虚拟地址计算的页目录项
  }
  phyaddr_t pte_phy = PTE_ADDR(pde_ptr[PDX(laddr)]);  // 计算页目录项
  uintptr_t *pte_ptr =
      (uintptr_t *)pte_phy;  // 获取页目录项的内容，即页表的起始地址
  // PTE写入page的地址
  phyaddr_t page_phy;
  if ((pte_ptr[PTX(laddr)] & PG_P) != 0) return;
  // 若页表项对应为空，则写入物理地址及对应的权限
  pte_ptr[PTX(laddr)] = paddr | pte_flag;
}
void map(u32 cr3, u32 size) {
  // 映射0～size
  for (phyaddr_t paddr = 0; paddr < size; paddr += PGSIZE) {
    lin_mapping_phy(cr3, paddr, paddr, PG_P | PG_USU | PG_RWW);
  }
  // 映射3G～3G+size
  for (phyaddr_t paddr = 0; paddr < size; paddr += PGSIZE) {
    lin_mapping_phy(cr3, K_PHY2LIN(paddr), paddr, PG_P | PG_USU | PG_RWW);
  }
}
