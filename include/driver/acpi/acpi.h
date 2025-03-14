#pragma once

#include <driver/acpi/lai/core.h> // IWYU pragma: export

void init_acpi();

enum {
    ACPI_VISIT_CONTINUE,
    ACPI_VISIT_SKIP,
    ACPI_VISIT_STOP,
};

typedef int (*acpi_sdst_visit_t)(lai_nsnode_t *, lai_state_t *, void *);
void acpi_sdst_visit(acpi_sdst_visit_t visit, void *arg);

typedef int (*acpi_device_visit_t)(lai_nsnode_t *, lai_state_t *, void *, void *);
void acpi_find_device(const char *name, acpi_device_visit_t visit, void *arg);

//! TODO: delete later, only for helpful device insight now
void scan_acpi_sdst();
