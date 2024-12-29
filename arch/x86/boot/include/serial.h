#pragma once

#include <stdint.h>
#include <stdbool.h>

bool serial_init();

uint8_t serial_read();
void serial_write(uint8_t byte);
