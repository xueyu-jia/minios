#include <driver/pci/pci.h>
#include <driver/usb/ehci.h>
#include <klib/stddef.h>

#define PCI_SUBCLS_USB_CTRL 0x03
#define PCI_PROGIF_EHCI 0x20

bool do_pci_ehci_init(pci_device_info_t *info, void *arg) {
    UNUSED(arg);
    if (info->class_code != PCI_CLASS_SERIAL_BUS_CTRL ||
        info->subclass != PCI_SUBCLS_USB_CTRL ||
        info->prog_if != PCI_PROGIF_EHCI) {
        return true;
    }

    // TODO: init ehci

    return false;
}

void ehci_init() {
    pci_visit(do_pci_ehci_init, NULL);
}
