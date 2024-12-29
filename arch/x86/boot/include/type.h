#pragma once

#include <stdint.h>
#include <stddef.h> // IWYU pragma: export

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t i32;

typedef u32 phyaddr_t;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef char CHAR;

#define PUBLIC
#define PRIVATE static

#define u2ptr(x) ((void*)(uintptr_t)(x))
#define ptr2u(x) ((uintptr_t)(void*)(x))

#define ROUNDDOWN(a, n)             \
    ({                              \
        u32 __a = (u32)(a);         \
        (typeof(a))(__a - __a % n); \
    })

#define ROUNDUP(a, n)                                    \
    ({                                                   \
        u32 __n = (u32)(n);                              \
        (typeof(a))(ROUNDDOWN((u32)(a) + __n - 1, __n)); \
    })
