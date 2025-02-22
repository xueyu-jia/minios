#pragma once

#include_next <stddef.h> // IWYU pragma: export

#define SZ_1 0x00000001
#define SZ_2 0x00000002
#define SZ_4 0x00000004
#define SZ_8 0x00000008
#define SZ_16 0x00000010
#define SZ_32 0x00000020
#define SZ_64 0x00000040
#define SZ_128 0x00000080
#define SZ_256 0x00000100
#define SZ_512 0x00000200

#define SZ_1K 0x00000400
#define SZ_2K 0x00000800
#define SZ_4K 0x00001000
#define SZ_8K 0x00002000
#define SZ_16K 0x00004000
#define SZ_32K 0x00008000
#define SZ_64K 0x00010000
#define SZ_128K 0x00020000
#define SZ_256K 0x00040000
#define SZ_512K 0x00080000

#define SZ_1M 0x00100000
#define SZ_2M 0x00200000
#define SZ_4M 0x00400000
#define SZ_8M 0x00800000
#define SZ_16M 0x01000000
#define SZ_32M 0x02000000
#define SZ_64M 0x04000000
#define SZ_128M 0x08000000
#define SZ_256M 0x10000000
#define SZ_512M 0x20000000

#define SZ_1G 0x40000000

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
//! kernel mmap:        0xffbff000~0xfffff000 4G-8M      ~ 4G-4M

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

#define SHM_ADDR ShareLinBase

#define MAXARG ((ArgLinLimitMAX - ArgLinBase) / 4 - 2)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef char CHAR;

typedef unsigned char* PBYTE;
typedef unsigned short* PWORD;
typedef unsigned long* PDWORD;
typedef unsigned int* PUINT;
typedef char* PCHAR;
