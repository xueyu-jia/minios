#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <compiler.h>
#include <macro_helper.h>

//! \see https://wiki.osdev.org/PCI

#define PCI_CONFIG_ADDR_REG 0xcf8
#define PCI_CONFIG_DATA_REG 0xcfc

#define PCI_BUS_SHIFT 16
#define PCI_DEV_SHIFT 11
#define PCI_FUNC_SHIFT 8

#define PCI_BUS_MASK 0xff
#define PCI_DEV_MASK 0x1f
#define PCI_FUNC_MASK 0x7
#define PCI_REG_MASK 0xfc

#define PCI_ID_MASK                                                      \
    ((PCI_BUS_MASK << PCI_BUS_SHIFT) | (PCI_DEV_MASK << PCI_DEV_SHIFT) | \
     (PCI_FUNC_MASK << PCI_FUNC_SHIFT))

#define PCI_MAX_BUS PCI_BUS_MASK
#define PCI_MAX_DEV PCI_DEV_MASK
#define PCI_MAX_FUNC PCI_FUNC_MASK

#define BIT(n) (1 << (n))

#define PCI_CONFIG_ENABLE BIT(31)
#define PCI_CONFIG_BUS(bus) (((bus) & PCI_BUS_MASK) << PCI_BUS_SHIFT)
#define PCI_CONFIG_DEV(dev) (((dev) & PCI_DEV_MASK) << PCI_DEV_SHIFT)
#define PCI_CONFIG_FUNC(func) (((func) & PCI_FUNC_MASK) << PCI_FUNC_SHIFT)
#define PCI_CONFIG_REG(reg) ((reg) & PCI_REG_MASK)

#define PCI_GET_BUS(addr) (((addr) >> PCI_BUS_SHIFT) & PCI_BUS_MASK)
#define PCI_GET_DEV(addr) (((addr) >> PCI_DEV_SHIFT) & PCI_DEV_MASK)
#define PCI_GET_FUNC(addr) (((addr) >> PCI_FUNC_SHIFT) & PCI_FUNC_MASK)

//! NOTE: reg is actually refers to the offset in the config space, the addr
//! passed to addr reg must be align to a dword, while you can use the least
//! 2-bits when accessing from the data reg

#define PCI_MAKE_ID(bus, dev, func) \
    (PCI_CONFIG_BUS(bus) | PCI_CONFIG_DEV(dev) | PCI_CONFIG_FUNC(func))

#define PCI_MAKE_ADDR_ARG2(id, reg) (PCI_CONFIG_ENABLE | ((id) & PCI_ID_MASK) | PCI_CONFIG_REG(reg))
#define PCI_MAKE_ADDR_ARG4(bus, dev, func, reg) \
    (PCI_CONFIG_ENABLE | PCI_MAKE_ID(bus, dev, func) | PCI_CONFIG_REG(reg))
#define PCI_MAKE_ADDR_IMPL(N, ...) MH_EXPAND(MH_CONCAT(PCI_MAKE_ADDR_ARG, N)(__VA_ARGS__))
#define PCI_MAKE_ADDR(...) \
    MH_EXPAND(PCI_MAKE_ADDR_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))

#define PCI_INVALID_VENDOR_ID 0xffff

enum pci_header_type {
    PCI_HDRTYPE_GENERIC = 0x00,        //<! generic device
    PCI_HDRTYPE_PCI_BRIDGE = 0x01,     //<! pci-to-pci bridge
    PCI_HDRTYPE_CARDBUS_BRIDGE = 0x02, //<! pci-to-cardbus bridge
};

#define PCI_HDR_GET_TYPE(hdr) ((hdr) & 0x7f)
#define PCI_HDR_HAS_MULTIFUNC(hdr) (((hdr) >> 7) & 0x1)

//! offset (in bytes) of each field in pci config space for each header type
//! public fields
#define PCI_CONFIG_VENDOR_ID 0x00
#define PCI_CONFIG_DEVICE_ID 0x02
#define PCI_CONFIG_COMMAND 0x04
#define PCI_CONFIG_STATUS 0x06
#define PCI_CONFIG_REVISION_ID 0x08
#define PCI_CONFIG_PROG_IF 0x09
#define PCI_CONFIG_SUBCLASS 0x0a
#define PCI_CONFIG_CLASS_CODE 0x0b
#define PCI_CONFIG_CACHELINE_SIZE 0x0c
#define PCI_CONFIG_LATENCY 0x0d
#define PCI_CONFIG_HEADER_TYPE 0x0e
#define PCI_CONFIG_BIST 0x0f
//! private fields for generic device
#define PCI_CONFIG_HDR0_BAR0 0x10
#define PCI_CONFIG_HDR0_BAR1 0x14
#define PCI_CONFIG_HDR0_BAR2 0x18
#define PCI_CONFIG_HDR0_BAR3 0x1c
#define PCI_CONFIG_HDR0_BAR4 0x20
#define PCI_CONFIG_HDR0_BAR5 0x24
#define PCI_CONFIG_HDR0_CARDBUS_CIS 0x28
#define PCI_CONFIG_HDR0_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_CONFIG_HDR0_SUBSYSTEM_DEVICE_ID 0x2e
#define PCI_CONFIG_HDR0_EXPANSION_ROM 0x30
#define PCI_CONFIG_HDR0_CAPABILITIY_PTR 0x34
#define PCI_CONFIG_HDR0_INTERRUPT_LINE 0x3c
#define PCI_CONFIG_HDR0_INTERRUPT_PIN 0x3d
#define PCI_CONFIG_HDR0_MIN_GRANT 0x3e
#define PCI_CONFIG_HDR0_MAX_LATENCY 0x3f
//! private fields for pci-to-pci bridge
#define PCI_CONFIG_HDR1_BAR0 0x10
#define PCI_CONFIG_HDR1_BAR1 0x14
#define PCI_CONFIG_HDR1_PRIMARY_BUS_NUM 0x18
#define PCI_CONFIG_HDR1_SECONDARY_BUS_NUM 0x19
#define PCI_CONFIG_HDR1_SUBORDINATE_BUS_NUN 0x1a
#define PCI_CONFIG_HDR1_SECONDARY_LATENCY 0x1b
#define PCI_CONFIG_HDR1_IO_BASE_LO8 0x1c
#define PCI_CONFIG_HDR1_IO_LIMIT_LO8 0x1d
#define PCI_CONFIG_HDR1_SECONDARY_STATUS 0x1e
#define PCI_CONFIG_HDR1_MEMORY_BASE 0x20
#define PCI_CONFIG_HDR1_MEMORY_LIMIT 0x22
#define PCI_CONFIG_HDR1_PREFETCHABLE_MEMORY_BASE_LO16 0x24
#define PCI_CONFIG_HDR1_PREFETCHABLE_MEMORY_LIMIT_LO16 0x26
#define PCI_CONFIG_HDR1_PREFETCHABLE_MEMORY_BASE_HI32 0x28
#define PCI_CONFIG_HDR1_PREFETCHABLE_MEMORY_LIMIT_HI32 0x2c
#define PCI_CONFIG_HDR1_IO_BASE_HI16 0x30
#define PCI_CONFIG_HDR1_IO_LIMIT_HI16 0x32
#define PCI_CONFIG_HDR1_CAPABILITIY_PTR 0x34
#define PCI_CONFIG_HDR1_EXPANSION_ROM 0x38
#define PCI_CONFIG_HDR1_INTERRUPT_LINE 0x3c
#define PCI_CONFIG_HDR1_INTERRUPT_PIN 0x3d
#define PCI_CONFIG_HDR1_BRIDGE_CTRL 0x3e
//! TODO: private fields for pci-to-cardbus bridge

enum pci_device_class {
    PCI_CLASS_UNCLASSIFIED = 0x00,        //<! unclassified
    PCI_CLASS_MASS_STORAGE_CTRL = 0x01,   //<! mass storage controller
    PCI_CLASS_NETWORK_CTRL = 0x02,        //<! network controller
    PCI_CLASS_DISPLAY_CTRL = 0x03,        //<! display controller
    PCI_CLASS_MULTIMEDIA_CTRL = 0x04,     //<! multimedia controller
    PCI_CLASS_MEMORY_CTRL = 0x05,         //<! memory controller
    PCI_CLASS_BRIDGE = 0x06,              //<! bridge
    PCI_CLASS_SIMPLE_COMM_CTRL = 0x07,    //<! simple communication controller
    PCI_CLASS_BASE_SYSTEM_PERIPH = 0x08,  //<! base system peripheral
    PCI_CLASS_INPUT_DEV_CTRL = 0x09,      //<! input device controller
    PCI_CLASS_DOCKING_STATION = 0x0a,     //<! docking station
    PCI_CLASS_PROCESSOR = 0x0b,           //<! processor
    PCI_CLASS_SERIAL_BUS_CTRL = 0x0c,     //<! serial bus controller
    PCI_CLASS_WIRELESS_CTRL = 0x0d,       //<! wireless controller
    PCI_CLASS_INTELLI_CTRL = 0x0e,        //<! intelligent controller
    PCI_CLASS_SATELLITE_COMM_CTRL = 0x0f, //<! satellite communication controller
    PCI_CLASS_ENCRYTION_CTRL = 0x10,      //<! encryption controller
    PCI_CLASS_SIGNAL_PROC_CTRL = 0x11,    //<! signal processing controller
    PCI_CLASS_PROC_ACC = 0x12,            //<! processing accelerator
    PCI_CLASS_NON_ESSENTIAL = 0x13,       //<! non-essential instrumentation
    PCI_CLASS_COPROCESSOR = 0x40,         //<! co-processor
    PCI_CLASS_UNASSIGNED = 0xff,          //<! unassigned class (vendor specific)
};

typedef struct pci_device_info {
    uint32_t id;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_info_t;

static inline bool pci_is_reserved_class(uint8_t class_code) {
    return class_code >= 0x14 && class_code <= 0xfe && class_code != PCI_CLASS_COPROCESSOR;
}

uint8_t pci_io_r8(int id, int reg);
uint16_t pci_io_r16(int id, int reg);
uint32_t pci_io_r32(int id, int reg);

void pci_io_w8(int id, int reg, uint8_t v);
void pci_io_w16(int id, int reg, uint16_t v);
void pci_io_w32(int id, int reg, uint32_t v);

bool pci_read_device_info(int id, pci_device_info_t *info);

typedef bool (*cb_pci_visit_t)(pci_device_info_t *, void *);
void pci_visit(cb_pci_visit_t visit, void *arg);
