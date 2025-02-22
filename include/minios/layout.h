#pragma once

#include <minios/page.h>
#include <klib/size.h>
#include <klib/sys/types.h>

//! user space:         0x00000000~0xc0000000 0          ~ 3G
//! user text:          0x00000000~0x20000000 0          ~ 512M
//! user data:          0x20000000~0x40000000 512M       ~ 1G
//! user reserved:      0x40000000~0x47fff000 1G         ~ 1G+128M-4K
//! user shared page:   0x47fff000~0x48000000 1G+128M-4K ~ 1G+128M
//! user heap:          0x48000000~0x87d00000 1G+128M    ~ 2G+125M
//! user shared memory: 0x87e00000~0x87f00000 2G+126M    ~ 2G+127M
//! user stack:         0x88000000~0xbffff000 2G+128M    ~ 3G-4K
//! user argv:          0xbffff000~0xc0000000 3G-4K      ~ 3G
//! kernel space:       0xc0000000~0xffffffff 3G         ~ 4G
//! kernel mmap:        0xff800000~0xffc00000 4G-8M      ~ 4G-4M

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
#define KernelLinMapLimit (KernelLinBase - SZ_4M + SZ_1G)
#define KernelLinMapBase (KernelLinMapLimit - (KernelLinMapMaxPage * SZ_4K))
#define KernelLinLimitMAX KernelLinMapBase

#define K_PHY2LIN(x) ((void *)((phyaddr_t)(x) + KernelLinBase))
#define K_LIN2PHY(x) ((phyaddr_t)((void *)(x) - KernelLinBase))
