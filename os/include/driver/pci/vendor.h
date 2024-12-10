#pragma once

#include <klib/stdbool.h>

typedef struct pci_vendor_info {
  int vendor_id;
  int device_id;
  const char *vendor_name;
  const char *device_name;
} pci_vendor_info_t;

bool pci_lookup_vendor(int vendor_id, int device_id, pci_vendor_info_t *info);
