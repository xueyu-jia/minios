#include <sys/elf.h>
#include <type.h>
#include <terminal.h>
#include <string.h>
#include <serial.h>
#include <paging.h>
#include <memory.h>
#include <global.h>
#include <layout.h>
#include <fs.h>
#include <fmt.h>
#include <disk.h>
#include <compiler.h>
#include <assert.h>
#include <ahci.h>
#include <disk.h>
#include <multiboot.h>

static void report_system_memory_map(const ards_t *ards_list, size_t total_ards) {
    const size_t arch_hex_width = sizeof(void *) * 2;
    char fmtbuf[64] = {};
    nstrfmt(fmtbuf, sizeof(fmtbuf),
            "ARDS[%%02d] base=0x%%0%dp limit=0x%%0%dp size=%%.2fMB type=%%s\n", arch_hex_width,
            arch_hex_width);

    lprintf("info: report system memory map\n");
    for (size_t i = 0; i < total_ards; ++i) {
        const ards_t *ards = &ards_list[i];
        const size_t size = ((u64)ards->size_hi << 32) | ards->size_lo;
        const phyaddr_t base = ((u64)ards->base_hi << 32) | ards->base_lo;
        //! NOTE: calc rhs first to avoid overflow
        const phyaddr_t limit = base + (size - 1);
        lprintf(fmtbuf, i, base, limit, size * 1. / MB, get_ards_type_str(ards->type));
    }
}

static void set_current_partition(size_t partition_lba) {
    boot_part_lba = partition_lba;
}

static phyaddr_t prepare_multiboot_info() {
    auto fmi = __fmi_ptr();
    phyaddr_t mbi_base = 0;

    const size_t rough_mmap_size = fmi->count * sizeof(multiboot_mmap_entry_t);
    const size_t rough_mbi_size = sizeof(multiboot_boot_info_t) + rough_mmap_size;
    for (size_t i = 0; i < fmi->count; ++i) {
        auto range = &fmi->page_ranges[i];
        const size_t seg_size = range->limit - range->base;
        if (range->limit <= SZ_1M) { continue; }
        if (seg_size < rough_mbi_size) { continue; }
        mbi_base = range->base;
        range->base = ROUNDUP(range->base + rough_mbi_size, PGSIZE);
        if (range->base == range->limit) {
            for (size_t j = i + 1; j < fmi->count; ++j) {
                fmi->page_ranges[j - 1] = fmi->page_ranges[j];
            }
            --fmi->count;
        }
        break;
    }
    assert(mbi_base != 0 && "no enough memory for multiboot info");

    multiboot_boot_info_t *mbi = u2ptr(mbi_base);
    lprintf("info: build multiboot boot info at 0x%p~0x%p\n", mbi_base,
            mbi_base + sizeof(multiboot_boot_info_t));
    memset(mbi, 0, sizeof(multiboot_boot_info_t));

    //! enable memory map
    multiboot_mmap_entry_t *mmap = (void *)&mbi[1];
    lprintf("info: build multiboot mmap at 0x%p~0x%p\n", mmap, mmap + fmi->count);
    mbi->flags |= BIT(6);
    mbi->mmap_length = fmi->count * sizeof(multiboot_mmap_entry_t);
    mbi->mmap_addr = ptr2u(mmap);
    for (size_t i = 0; i < fmi->count; ++i) {
        mmap[i].size = sizeof(multiboot_mmap_entry_t) - sizeof(mmap[i].size);
        mmap[i].base_addr = fmi->page_ranges[i].base;
        mmap[i].length = fmi->page_ranges[i].limit - fmi->page_ranges[i].base;
        mmap[i].type = MULTIBOOT_MEMORY_AVAILABLE;
    }

    return ptr2u(mbi);
}

NORETURN static void load_kernel(size_t total_memory) {
    lprintf("info: load kernel elf\n");

    const bool fs_inited = init_fs();
    assert(fs_inited && "failed to init bootfs");

    do {
        open_file(KERNEL_FILENAME);

        elf32_hdr_t eh = {};
        bool noerr = read(0, sizeof(elf32_hdr_t), (void *)&eh);
        if (!noerr) { break; }

        elf32_phdr_t *ph = u2ptr(ELF_ADDR);
        const size_t ph_size = eh.e_phentsize * eh.e_phnum;
        assert(ph_size <= ELF_BUF_LEN && "elf program headers too large");

        noerr = noerr && read(eh.e_phoff, eh.e_phentsize * eh.e_phnum, (void *)ph);

        uintptr_t min_elf_va = ~(uintptr_t)0;
        uintptr_t max_elf_va = 0;
        for (int i = 0; i < eh.e_phnum; ++i) {
            if (ph[i].p_type == PT_LOAD) {
                min_elf_va = MIN(min_elf_va, ph[i].p_vaddr);
                max_elf_va = MAX(max_elf_va, ph[i].p_vaddr + ph[i].p_memsz);
            }
        }
        //! NOTE: the assertion is not that accurate, but it's enough here
        assert((size_t)K_LIN2PHY(max_elf_va) < total_memory && "no enough memory for kernel elf");

        for (int i = 0; i < eh.e_phnum; ++i) {
            if (ph[i].p_type != PT_LOAD) { continue; }
            lprintf("load segment %d: va=0x%p, offset=0x%p, filesz=0x%x, memsz=0x%x\n", i,
                    ph[i].p_vaddr, ph[i].p_offset, ph[i].p_filesz, ph[i].p_memsz);
            noerr = noerr && read(ph[i].p_offset, ph[i].p_filesz, (void *)ph[i].p_vaddr);
            memset((void *)ph[i].p_vaddr + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);
        }

        if (!noerr) { break; }

        lprintf("info: kernel loaded\n");

        //! exclude critical physical memory ranges
        {
            const phyaddr_t base = PageDirBase;
            const phyaddr_t limit = base + get_page_table_usage();
            lprintf("info: exclude physical range 0x%p~0x%p occupied by page table of loader\n",
                    base, limit);
            exclude_physical_range(base, limit);
        }
        {
            const phyaddr_t base = ROUNDDOWN(K_LIN2PHY(min_elf_va), PGSIZE);
            const phyaddr_t limit = ROUNDUP(K_LIN2PHY(max_elf_va) + 1, PGSIZE);
            lprintf("info: exclude physical range 0x%p~0x%p occupied by kernel elf\n", base, limit);
            exclude_physical_range(base, limit);
        }

        const phyaddr_t mb_phy_addr = prepare_multiboot_info();

        lprintf("info: after exclusion and multiboot info setup\n");
        for (size_t i = 0; i < __fmi_ptr()->count; ++i) {
            lprintf("[%02d] 0x%p~0x%p\n", i, __fmi_ptr()->page_ranges[i].base,
                    __fmi_ptr()->page_ranges[i].limit);
        }

        asm volatile("call *%0" ::"r"(eh.e_entry), "d"(mb_phy_addr));
    } while (0);

    abort("fatal: failed to load kernel elf\n");
}

void cstart(size_t partition_lba, ards_t *ards_list, size_t total_ards) {
    //! NOTE: take account of the fact that vga buffer is actually a mmio memory and will not be
    //! reported in ards list, we simply ignore it and use the port io uart instead throughout the
    //! boot process, graphics device like vga will be initialized later in the kernel through a
    //! regular device probe-and-init procedure
    //! NOTE: gs and descriptor table are also not initialized for video memory, do not try to
    //! access it for now; meanwhile, remember to handle it in kernel if needed

    const bool serial_inited = serial_init();
    assert(serial_inited && "failed to init uart device for serial output in loader");

    lprintf("info: current partition lba: %d\n", partition_lba);
    set_current_partition(partition_lba);

    report_system_memory_map(ards_list, total_ards);

    const size_t free_pages = probe_memory(ards_list, total_ards);
    assert(free_pages > 0 && "physical memory is not available");

    lprintf("info: found %d free pages, about %.2fMB physical memory available in total\n",
            free_pages, free_pages * 1. * PGSIZE / MB);
    assert(free_pages * PGSIZE >= NUM_1M * 8 && "physical memory is too small");

    setup_paging();

    ahci_sata_init();

    load_kernel(free_pages * PGSIZE);
}
