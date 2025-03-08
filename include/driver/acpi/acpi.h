#pragma once

#include <driver/acpi/lai/core.h> // IWYU pragma: export

void init_acpi();

//! TODO: delete later, only for helpful device insight now
void scan_acpi_sdst();
