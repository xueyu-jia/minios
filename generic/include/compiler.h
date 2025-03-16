#pragma once

#include <macro_helper.h>

#if defined(__clang__)
#define CLANG_COMPILER 1
#elif defined(__GNUC__)
#define GCC_COMPILER 1
#elif defined(_MSC_VER)
#define MSVC_COMPILER 1
#endif

#if __PTRDIFF_WIDTH__ == 32
#define ARCH_32 1
#elif __PTRDIFF_WIDTH__ == 64
#define ARCH_64 1
#endif

#define asm __asm__
#define typeof __typeof__
#define auto __auto_type

#define static_assert(expr, msg) _Static_assert(expr, msg)

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define is_same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)
#define container_of(ptr, type, member)                      \
    ({                                                       \
        const typeof(((type *)0)->member) *__mptr = (ptr);   \
        (type *)((char *)(__mptr) - offsetof(type, member)); \
    })

#define MAYBE_UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))
#define ALIGN(n) __attribute__((aligned(n)))
#define NAKED __attribute__((naked))

#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))

#define MARK_AS_UNUSED(x) ((void)(x))
#define UNUSED_IMPL(x) MH_EXPAND(MH_PAIR(;, MARK_AS_UNUSED(x)))
#define UNUSED(x, ...) MARK_AS_UNUSED(x) MH_FOREACH(UNUSED_IMPL, __VA_ARGS__)

#define MUST_USE_RESULT __attribute__((warn_unused_result))

#if defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH ((void)0)
#endif
