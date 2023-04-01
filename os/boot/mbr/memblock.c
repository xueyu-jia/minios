//ported by sundong 2023.3.26
#include "loaderprint.h"
#include "memblock.h"
void mem_check_32M() {
  lprintf("----start memblock----\n");
  // 禁止缓存
  u32 flag;
  flag = rcr0();
  flag = flag | 0x60000000;  // 设置CD,NW位 cache disabled
  lcr0(flag);
  u32 startaddr;
  startaddr = START_ADDR;  // 起始地址0
  u32 endaddr;
  endaddr = END_ADDR;  // 结尾地址32M
  u32* MemInfo;
  MemInfo = (u32*)FMIBuff;  // 存放结果的地址
  u32 count = 0;

  while (startaddr <= endaddr)  // 循环检查
  {
    count++;
    u32 addr;
    addr = mem_test(startaddr, endaddr);  // 检查该地址起始的内存块是否可用
    MemInfo[count] = addr;  // 返回检查到的连续可用的内存的最后的地址写入数组中
    startaddr = addr + CHECK_SIZE;  // 每次检查0x1000
  }
  lprintf("memblock count %d\n", count);
  MemInfo[0] =
      count;  // 循环次数即free的内存块数量写入结果数组的偏移量为0的地方
  flag = rcr0();
  flag = flag & 0x9fffffff;  // 清除CD,NW位，恢复缓存
  lcr0(flag);
  lprintf("----finish memblock----\n");
  // while(1);
  return;
}
// 检查起始地址往后的内存块是否可用
u32 mem_test(u32 startaddr, u32 endaddr) {
  u32 start = startaddr;
  while (start <= endaddr) {
    u32 check_4b = start + 0xffc;  // 检查最后4B
    u32* check_4b_ptr = (u32*)check_4b;
    u32 origin_check_4b = (*check_4b_ptr);
    *check_4b_ptr = CHECK_ORIGIN;  // 保存原值
    // 反转
    *check_4b_ptr = (*check_4b_ptr) ^ (0xffffffff);
    if ((*check_4b_ptr) != CHECK_REVERSE) {
      *check_4b_ptr = origin_check_4b;  // 写回原值
      return start;                     // 返回检查的起始地址
    }
    // 再次反转
    *check_4b_ptr = (*check_4b_ptr) ^ (0xffffffff);
    if ((*check_4b_ptr) != CHECK_ORIGIN) {
      *check_4b_ptr = origin_check_4b;  // 写回原值
      return start;                     // 返回检查的起始地址
    }
    *check_4b_ptr = origin_check_4b;  // 都通过则写回原值
    start = start + CHECK_SIZE;       // 每次检查0x1000
  }
  return start;
}