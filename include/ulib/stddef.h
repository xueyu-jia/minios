#pragma once

#include_next <stddef.h> // IWYU pragma: export

#define num_4B 0x4
#define num_1K 0x400
#define num_4K 0x1000
#define num_4M 0x400000

#define TextLinBase ((uintptr_t)0x0)
#define TextLinLimitMAX (TextLinBase + 0x20000000)
#define DataLinBase TextLinLimitMAX
#define DataLinLimitMAX (DataLinBase + 0x20000000)
#define VpageLinBase DataLinLimitMAX
#define VpageLinLimitMAX (VpageLinBase + 0x8000000 - num_4K)
#define SharePageBase VpageLinLimitMAX
#define SharePageLimit (SharePageBase + num_4K)
#define HeapLinBase SharePageLimit
#define HeapLinLimitMAX (HeapLinBase + 0x3FD00000)
#define ShareLinBase (HeapLinLimitMAX + 0x100000)
#define ShareLinLimitMAX (ShareLinBase + 0x100000)
#define KernelLinBase 0xc0000000
#define ArgLinLimitMAX KernelLinBase
#define ArgLinBase ((ArgLinLimitMAX) - 0x1000)

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
