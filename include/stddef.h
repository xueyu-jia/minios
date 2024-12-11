#pragma once

#include <stdint.h>

#define NULL ((void*)0)

typedef uint32_t size_t;
typedef int32_t ssize_t;

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t ptrdiff_t;

#define min(x, y)           \
    ({                      \
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        (void)(&_x == &_y); \
        _x < _y ? _x : _y;  \
    })

#define max(x, y)           \
    ({                      \
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        (void)(&_x == &_y); \
        _x > _y ? _x : _y;  \
    })
