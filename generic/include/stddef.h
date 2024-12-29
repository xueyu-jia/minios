#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t size_t;
typedef int32_t ssize_t;

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t ptrdiff_t;

#define NULL ((void*)0)

#define TRUE true
#define FALSE false

#define MIN(x, y)           \
    ({                      \
        typeof(x) _x = (x); \
        typeof(x) _y = (y); \
        (void)(&_x == &_y); \
        _x < _y ? _x : _y;  \
    })

#define MAX(x, y)           \
    ({                      \
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        (void)(&_x == &_y); \
        _x > _y ? _x : _y;  \
    })

#define CLAMP(x, lo, hi) MIN(MAX(lo, x), hi)

#define ARRAY_SIZE(x) ((ssize_t)(sizeof(x) / sizeof((x)[0])))
