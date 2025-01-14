#pragma once

#include <minios/page.h>
#include <klib/size.h>
#include <klib/sys/types.h>

//! FIXME: 早晚杀了你
#define PHY_MEM_SIZE SZ_64M

#define KernelPageTblAddr SZ_1M

#define TextLinBase ((uintptr_t)(0x0))
#define TextLinLimitMAX (TextLinBase + SZ_512M)
#define DataLinBase TextLinLimitMAX
#define DataLinLimitMAX (DataLinBase + SZ_512M)
#define VpageLinBase DataLinLimitMAX
#define VpageLinLimitMAX (VpageLinBase + SZ_128M - SZ_4K)
#define SharePageBase VpageLinLimitMAX
#define SharePageLimit (SharePageBase + SZ_4K)
#define HeapLinBase SharePageLimit
#define HeapLinLimitMAX (HeapLinBase + SZ_1G - SZ_1M * 3u)
#define ShareLinBase (HeapLinLimitMAX + SZ_1M)
#define ShareLinLimitMAX (ShareLinBase + SZ_1M)
#define KernelLinBase ((uintptr_t)(SZ_1G * 3u))

#define ArgLinLimitMAX KernelLinBase
#define ArgLinBase (ArgLinLimitMAX - SZ_4K)

#define StackLinBase ArgLinBase
#define StackLinLimitMAX (ShareLinLimitMAX + SZ_1M)

#define KernelLinMapMaxPage 1024
#define KernelLinMapBase (KernelLinBase + PHY_MEM_SIZE)
#define KernelLinMapLimit (KernelLinMapBase + (KernelLinMapMaxPage * PGSIZE))
#define KernelLinLimitMAX (KernelLinBase + SZ_1G)

#define SmallKernelSize SZ_8M
#define BigKernelSize SZ_32M

#define K_PHY2LIN(x) ((void *)((phyaddr_t)(x) + KernelLinBase))
#define K_LIN2PHY(x) ((phyaddr_t)((void *)(x) - KernelLinBase))
