#pragma once

#ifndef asm
#define asm __asm__
#endif

#ifndef typeof
#define typeof __typeof__
#endif

#define auto __auto_type

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define is_same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define static_assert(expr, msg) _Static_assert(expr, msg)

#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

#define container_of(ptr, type, member)                      \
    ({                                                       \
        const typeof(((type *)0)->member) *__mptr = (ptr);   \
        (type *)((char *)(__mptr) - offsetof(type, member)); \
    })

#define MAYBE_UNUSED __attribute__((unused))

#define PACKED __attribute__((packed))
