/*
 * @Author: Rong.Ding
 * @Date: 2023-01-13 23:11:17
 * @Last Modified by: Yuanhong.Yu
 * @Last Modified time: 2023-01-13 23:12:49
 */
// ported by sundong 2023.3.26

#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
#include "type.h"
#include "x86.h"

#define FMIBuff 0x007ff000
#define CHECK_REVERSE 0x55aa55aa
#define CHECK_ORIGIN 0xaa55aa55
#define CHECK_SIZE 0x1000
#define START_ADDR 0x00000000
#define END_ADDR 0x02000000

u32 get_mem_info(u32 MemChkBuf, u32 MCRNumber);

#endif
