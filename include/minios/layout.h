#pragma once

#include <klib/size.h>
#include <klib/sys/types.h>

#define KernelPageTblAddr SZ_1M

#define SmallKernelSize SZ_8M
#define BigKernelSize SZ_32M

#define KernelLinBase ((uintptr_t)(SZ_1G * 3u))

#define K_PHY2LIN(x) ((void *)((phyaddr_t)(x) + KernelLinBase))
#define K_LIN2PHY(x) ((phyaddr_t)((void *)(x) - KernelLinBase))
