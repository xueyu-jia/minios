#include <driver/pci/vendor.h>
#include <klib/compiler.h>

struct vendor_info {
    bool is_device;
    int id;
    const char *name;
};

struct vendor_info VENDOR_TABLE[] = {
#if 1
#include "./pci_vendor_table"
#endif
};

bool pci_lookup_vendor(int vendor_id, int device_id, pci_vendor_info_t *info) {
    const int N = sizeof(VENDOR_TABLE) / sizeof(struct vendor_info);
    int ivendor = 0;
    while (ivendor < N) {
        const auto v = &VENDOR_TABLE[ivendor];
        if (!v->is_device && v->id == vendor_id) { break; }
        ++ivendor;
    }
    if (ivendor == N) { return false; }
    int idevice = ivendor + 1;
    while (idevice < N) {
        const auto v = &VENDOR_TABLE[idevice];
        if (!v->is_device) {
            idevice = N;
            break;
        }
        if (v->id == device_id) { break; }
        ++idevice;
    }
    info->vendor_id = vendor_id;
    info->vendor_name = VENDOR_TABLE[ivendor].name;
    info->device_id = device_id;
    info->device_name = idevice == N ? "Unknown" : VENDOR_TABLE[idevice].name;
    return true;
}
