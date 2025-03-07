#include <lai/host.h>
#include <acpispec/tables.h>
#include <minios/memman.h>
#include <minios/assert.h>
#include <minios/ioremap.h>
#include <minios/console.h>
#include <minios/asm.h>
#include <driver/pci/pci.h>
#include <compiler.h>
#include <string.h>

static acpi_xsdp_t *__xsdp = NULL;
static acpi_rsdt_t *__rsdt = NULL;
static acpi_xsdt_t *__xsdt = NULL;
static size_t __nr_sdt_tables = 0;

static phyaddr_t (*__acpi_table_at)(int index) = NULL;

static phyaddr_t acpi_table_at_from_rsdt(int index) {
    assert(__rsdt);
    return __rsdt->tables[index];
}

static phyaddr_t acpi_table_at_from_xsdt(int index) {
    assert(__xsdt);
    return __xsdt->tables[index];
}

static void ensures_acpi_sdt_initialized() {
    if (__xsdp) { return; }
    typedef struct {
        phyaddr_t base;
        size_t size;
    } ds_t;
    ds_t scan_areas[2] = {
        {0x000e0000, SZ_128K}, //<! main bios area
        {0x00080000, SZ_1K},   //<! first 1KiB of EBDA
    };
    for (int i = 0; i < ARRAY_SIZE(scan_areas); ++i) {
        ds_t *area = &scan_areas[i];
        void *va = ioremap(area->base, area->size);
        const void *limit = va + area->size;
        void *ptr = va;
        while (ptr < limit) {
            if (memcmp(ptr, "RSD PTR ", 8) == 0) {
                __xsdp = ptr;
                break;
            }
            ptr += SZ_16;
        }
        if (__xsdp) { break; }
        iounmap(va);
    }
    assert(__xsdp);
    assert(__xsdp->revision == 0 || __xsdp->revision == 2);
    const phyaddr_t pa = __xsdp->revision == 0 ? __xsdp->rsdt : __xsdp->xsdt;
    acpi_header_t *hdr = ioremap(pa, sizeof(acpi_header_t));
    const size_t size = hdr->length;
    iounmap(hdr);
    void *va_sdt = ioremap(pa, size);
    if (__xsdp->revision == 0) {
        __rsdt = va_sdt;
        __nr_sdt_tables = (__rsdt->header.length - sizeof(acpi_header_t)) / 4;
        __acpi_table_at = acpi_table_at_from_rsdt;
    } else {
        __xsdt = va_sdt;
        __nr_sdt_tables = (__xsdt->header.length - sizeof(acpi_header_t)) / 8;
        __acpi_table_at = acpi_table_at_from_xsdt;
    }
}

void *acpi_find_sdt(const char *sign, int index) {
    assert(sign != NULL);
    int nxt_idx = 0;
    for (size_t i = 0; i < __nr_sdt_tables; ++i) {
        const phyaddr_t dt_pa = __acpi_table_at(i);
        acpi_header_t *hdr = ioremap(dt_pa, sizeof(acpi_header_t));
        if (memcmp(hdr->signature, sign, 4) == 0) {
            if (nxt_idx == index) {
                size_t len = hdr->length;
                iounmap(hdr);
                return ioremap(dt_pa, len);
            }
            ++nxt_idx;
        }
        iounmap(hdr);
    }
    return NULL;
}

void laihost_log(int level, const char *msg) {
    const char *prompt[] = {
        [LAI_DEBUG_LOG] = "info",
        [LAI_WARN_LOG] = "warn",
    };
    kprintf("%s: lai: %s\n", prompt[level], msg);
}

void laihost_panic(const char *msg) {
    abort("fatal: lai: %s\n", msg);
}

void *laihost_malloc(size_t len) {
    return kern_kmalloc(len);
}

void *laihost_realloc(void *ptr, size_t newsize, size_t oldsize) {
    void *new_ptr = kern_kmalloc(newsize);
    assert(new_ptr != NULL);
    if (ptr != NULL) {
        memmove(new_ptr, ptr, oldsize);
        kern_kfree(ptr);
    }
    return new_ptr;
}

void laihost_free(void *ptr, size_t len) {
    UNUSED(len);
    kern_kfree(ptr);
}

void *laihost_scan(const char *sign, size_t index) {
    ensures_acpi_sdt_initialized();
    assert(sign != NULL);

    if (memcmp(sign, "DSDT", 4) == 0) {
        acpi_fadt_t *fadt = acpi_find_sdt("FACP", 0);
        if (fadt == NULL) { return NULL; }
        const phyaddr_t dsdt_pa = __rsdt ? fadt->dsdt : fadt->x_dsdt;
        iounmap(fadt);
        acpi_header_t *hdr = ioremap(dsdt_pa, sizeof(acpi_header_t));
        const size_t len = hdr->length;
        iounmap(hdr);
        return ioremap(dsdt_pa, len);
    }

    return acpi_find_sdt(sign, index);
}

void laihost_outb(uint16_t port, uint8_t value) {
    outb(port, value);
}

void laihost_outw(uint16_t port, uint16_t value) {
    outw(port, value);
}

void laihost_outd(uint16_t port, uint32_t value) {
    outl(port, value);
}

uint8_t laihost_inb(uint16_t port) {
    return inb(port);
}

uint16_t laihost_inw(uint16_t port) {
    return inw(port);
}

uint32_t laihost_ind(uint16_t port) {
    return inl(port);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint8_t value) {
    UNUSED(seg);
    pci_io_w8(PCI_MAKE_ID(bbn, slot, fun), offset, value);
}

void laihost_pci_writew(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint16_t value) {
    UNUSED(seg);
    pci_io_w16(PCI_MAKE_ID(bbn, slot, fun), offset, value);
}

void laihost_pci_writed(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint32_t value) {
    UNUSED(seg);
    pci_io_w32(PCI_MAKE_ID(bbn, slot, fun), offset, value);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset) {
    UNUSED(seg);
    return pci_io_r8(PCI_MAKE_ID(bbn, slot, fun), offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset) {
    UNUSED(seg);
    return pci_io_r16(PCI_MAKE_ID(bbn, slot, fun), offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bbn, uint8_t slot, uint8_t fun, uint16_t offset) {
    UNUSED(seg);
    return pci_io_r32(PCI_MAKE_ID(bbn, slot, fun), offset);
}
