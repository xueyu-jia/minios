#include <memory.h>
#include <type.h>
#include <x86.h>
#include <regs.h>
#include <paging.h>
#include <compiler.h>
#include <assert.h>
#include <terminal.h>

#ifdef ARCH_32
#define FREE_MEMORY_BASE 0x00100000
#else
#error "memory probe is not supported for x86_64"
#endif

const char* get_ards_type_str(int type) {
    static const char* TYPE_TABLE[5] = {
        "unknown", "memory", "reserved", "acpi-reclaim", "acpi-nvs",
    };
    const int index = type >= 1 && type < ARRAY_SIZE(TYPE_TABLE) ? type : 0;
    return TYPE_TABLE[index];
}

size_t probe_memory(ards_t* ards_list, size_t total_ards) {
    static bool probed = false;
    static size_t cached_total_valid_pages = 0;
    if (probed) { return cached_total_valid_pages; }

    size_t total_valid_pages = 0;
    auto fmi = __fmi_ptr();
    fmi->count = 0;

    //! NOTE: ards list is assumed to be ascending ordered by base address
    //! NOTE: probe procedure is expected to stop when the standard free memory base is reached
    //! FIXME: ards may overlaps and can be unsorted
    phyaddr_t last_ards_limit = 0;
    for (size_t i = 0; i < total_ards; ++i) {
        const ards_t* ards = &ards_list[i];

        const phyaddr_t ards_base = ards->base;
        const size_t ards_size = ards->size;
        const phyaddr_t ards_limit = ards_base + ards_size;

        assert(ards_base >= last_ards_limit && "ards list is not ascending order by base address");
        last_ards_limit = ards_limit;

        if (ards->type != ARDS_TYPE_MEMORY) { continue; }

        const phyaddr_t page_base = ROUNDUP(ards_base, PGSIZE);
        const phyaddr_t page_limit = ROUNDDOWN(ards_limit, PGSIZE);

        const size_t index = fmi->count++;
        fmi->page_ranges[index].base = page_base;
        fmi->page_ranges[index].limit = page_limit;
        total_valid_pages += (page_limit - page_base) / PGSIZE;
    }

    probed = true;
    cached_total_valid_pages = total_valid_pages;
    return total_valid_pages;
}

void exclude_physical_range(phyaddr_t base, phyaddr_t limit) {
    //! TODO: mark as critical rather than exclude
    assert(PGOFF(base) == 0 && PGOFF(limit) == 0);
    if (base == limit) { return; }
    auto fmi = __fmi_ptr();
    size_t index = 0;
    while (index < fmi->count) {
        auto range = &fmi->page_ranges[index];
        if (range->base >= limit) { break; }
        if (range->limit <= base) {
            ++index;
            continue;
        }
        if (range->base < base && range->limit > limit) {
            ++fmi->count;
            for (size_t i = fmi->count - 1; i > index; --i) {
                fmi->page_ranges[i] = fmi->page_ranges[i - 1];
            }
            fmi->page_ranges[index].limit = base;
            fmi->page_ranges[index + 1].base = limit;
            return;
        }
        if (range->limit >= limit) {
            range->base = limit;
            break;
        }
        if (range->base <= base) {
            range->limit = base;
            ++index;
            continue;
        }
        unreachable();
    }
    size_t p = 0;
    size_t q = p;
    while (p < fmi->count) {
        auto range = &fmi->page_ranges[p];
        if (range->base < range->limit) {
            if (p != q) { fmi->page_ranges[q] = fmi->page_ranges[p]; }
            ++p;
            ++q;
        } else if (range->base == range->limit) {
            ++p;
        } else {
            unreachable();
        }
    }
    fmi->count = q;
}
