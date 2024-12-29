#pragma once

#include <stddef.h> // IWYU pragma: export
#include <macro_helper.h>

#define BIT(n) (1 << (n))

#define UINT_PTR_CONV_IMPL(conv, N, ...) MH_EXPAND(MH_CONCAT(conv, MH_CONCAT(_, N))(__VA_ARGS__))

#define UINT_PTR_CONV_WRAPPER(conv, ...) \
    MH_EXPAND(UINT_PTR_CONV_IMPL(conv, MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))

#define __u2ptr_1(x) ((void*)(uintptr_t)(x))
#define __u2ptr_2(type, x) ((type)__u2ptr_1(x))

#define __ptr2u_1(x) ((uintptr_t)(void*)(x))
#define __ptr2u_2(type, x) ((type)__ptr2u_1(x))

#define u2ptr(...) UINT_PTR_CONV_WRAPPER(__u2ptr, __VA_ARGS__)
#define ptr2u(...) UINT_PTR_CONV_WRAPPER(__ptr2u, __VA_ARGS__)

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
