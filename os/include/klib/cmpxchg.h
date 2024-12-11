/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-16 18:02:45
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 10:45:12
 * @FilePath: /minios/os/include/klib/cmpxchg.h
 * @Description:
        __cmpxchg函数的实现来自Linux源码，有稍许修改和简化
        参见https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/cmpxchg.h
 *
 */

#pragma once
#include <kernel/type.h>
#define MY_LOCK_PREFIX "lock;"

#define __X86_CASE_B 1
#define __X86_CASE_W 2
#define __X86_CASE_L 4
#ifdef CONFIG_64BIT
#define __X86_CASE_Q 8
#else
#define __X86_CASE_Q -1 /* sizeof will never return -1 */
#endif

#define __cmpxchg_wrong_size() \
    while (1) {}; // todo

#ifdef CONFIG_64BIT
#define __X86_CASE_Q 8
#define __raw_cmpxchg(ptr, old, new, size, lock)             \
    ({                                                       \
        __typeof__(*(ptr)) __ret;                            \
        __typeof__(*(ptr)) __old = (old);                    \
        __typeof__(*(ptr)) __new = (new);                    \
        switch (size) {                                      \
            case __X86_CASE_B: {                             \
                volatile u8 *__ptr = (volatile u8 *)(ptr);   \
                asm volatile(lock "cmpxchgb %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "q"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            case __X86_CASE_W: {                             \
                volatile u16 *__ptr = (volatile u16 *)(ptr); \
                asm volatile(lock "cmpxchgw %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "r"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            case __X86_CASE_L: {                             \
                volatile u32 *__ptr = (volatile u32 *)(ptr); \
                asm volatile(lock "cmpxchgl %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "r"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            case __X86_CASE_Q: {                             \
                volatile u64 *__ptr = (volatile u64 *)(ptr); \
                asm volatile(lock "cmpxchgq %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "r"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            default:                                         \
                __cmpxchg_wrong_size();                      \
        }                                                    \
        __ret;                                               \
    })
#else
#define __X86_CASE_Q -1 /* sizeof will never return -1 */
#define __raw_cmpxchg(ptr, old, new, size, lock)             \
    ({                                                       \
        __typeof__(*(ptr)) __ret;                            \
        __typeof__(*(ptr)) __old = (old);                    \
        __typeof__(*(ptr)) __new = (new);                    \
        switch (size) {                                      \
            case __X86_CASE_B: {                             \
                volatile u8 *__ptr = (volatile u8 *)(ptr);   \
                asm volatile(lock "cmpxchgb %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "q"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            case __X86_CASE_W: {                             \
                volatile u16 *__ptr = (volatile u16 *)(ptr); \
                asm volatile(lock "cmpxchgw %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "r"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            case __X86_CASE_L: {                             \
                volatile u32 *__ptr = (volatile u32 *)(ptr); \
                asm volatile(lock "cmpxchgl %2,%1"           \
                             : "=a"(__ret), "+m"(*__ptr)     \
                             : "r"(__new), "0"(__old)        \
                             : "memory");                    \
                break;                                       \
            }                                                \
            default:                                         \
                __cmpxchg_wrong_size();                      \
        }                                                    \
        __ret;                                               \
    })
#endif
#define __cmpxchg(ptr, old, new, size) \
    __raw_cmpxchg((ptr), (old), (new), (size), MY_LOCK_PREFIX)
