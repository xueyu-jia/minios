#pragma once

#include <stdint.h>
#include <compiler.h>

typedef struct {
    uint8_t desc;
    uint8_t info;
    uint16_t addr_base_min;
    uint16_t addr_base_max;
    uint8_t base_align;
    uint8_t length;
} PACKED acpi_resdt_io_port_t;
