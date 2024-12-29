#pragma once

#include <type.h>
#include <stdbool.h>
#include <compiler.h>

typedef struct free_memory_info {
    size_t count;
    struct {
        phyaddr_t base;
        phyaddr_t limit;
    } page_ranges[0];
} free_memory_info_t;

#define __fmi_ptr() ((free_memory_info_t *)u2ptr(0x007ff000))

typedef struct ards {
    union {
        struct {
            u32 base_lo;
            u32 base_hi;
        };
        u64 base;
    };
    union {
        struct {
            u32 size_lo;
            u32 size_hi;
        };
        u64 size;
    };
    u32 type;
} PACKED ards_t;

enum ards_type {
    ARDS_TYPE_MEMORY = 0x01,
    ARDS_TYPE_RESERVED = 0x02,
    ARDS_TYPE_ACPI_RECLAIM = 0x03,
    ARDS_TYPE_ACPI_NVS = 0x04,
};

const char *get_ards_type_str(int type);

size_t probe_memory(ards_t *ards_list, size_t total_ards);
void exclude_physical_range(phyaddr_t base, phyaddr_t limit);
