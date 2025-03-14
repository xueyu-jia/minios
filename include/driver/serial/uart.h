#pragma once

#include <stdint.h>
#include <stdbool.h>

int serial_uart_probe();

bool serial_uart_exists(int index);
uint8_t serial_uart_read(int index);
void serial_uart_write(int index, uint8_t byte);
