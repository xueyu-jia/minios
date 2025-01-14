#include <driver/pci/pci.h>
#include <minios/asm.h>

uint8_t pci_io_r8(int id, int reg) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    return inb(PCI_CONFIG_DATA_REG | (reg & 0x3));
}

uint16_t pci_io_r16(int id, int reg) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    return inw(PCI_CONFIG_DATA_REG | (reg & 0x2));
}

uint32_t pci_io_r32(int id, int reg) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    return inl(PCI_CONFIG_DATA_REG);
}

void pci_io_w8(int id, int reg, uint8_t v) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    outb(PCI_CONFIG_DATA_REG | (reg & 0x3), v);
}

void pci_io_w16(int id, int reg, uint16_t v) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    outw(PCI_CONFIG_DATA_REG | (reg & 0x2), v);
}

void pci_io_w32(int id, int reg, uint32_t v) {
    const uint32_t addr = PCI_MAKE_ADDR(id, reg);
    outl(PCI_CONFIG_ADDR_REG, addr);
    outl(PCI_CONFIG_DATA_REG, v);
}

bool pci_read_device_info(int id, pci_device_info_t *info) {
    if (info == NULL) { return false; }
    info->id = id;
    info->vendor_id = pci_io_r16(id, PCI_CONFIG_VENDOR_ID);
    info->device_id = pci_io_r16(id, PCI_CONFIG_DEVICE_ID);
    info->class_code = pci_io_r8(id, PCI_CONFIG_CLASS_CODE);
    info->subclass = pci_io_r8(id, PCI_CONFIG_SUBCLASS);
    info->prog_if = pci_io_r8(id, PCI_CONFIG_PROG_IF);
    return true;
}

void pci_visit(cb_pci_visit_t visit, void *arg) {
    if (visit == NULL) { return; }
    bool completed = false;
    pci_device_info_t info = {};
    for (int bus = 0; bus < PCI_MAX_BUS && !completed; ++bus) {
        for (int dev = 0; dev < PCI_MAX_DEV && !completed; ++dev) {
            for (int func = 0; func < PCI_MAX_FUNC && !completed; ++func) {
                const int id = PCI_MAKE_ID(bus, dev, func);
                const bool retrived = pci_read_device_info(id, &info);
                if (!retrived || info.vendor_id == PCI_INVALID_VENDOR_ID) { continue; }
                completed = !visit(&info, arg);
            }
        }
    }
}
