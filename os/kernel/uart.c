#include <kernel/proto.h>
#include <kernel/uart.h>
// port from uniform os
int init_simple_serial() {
    out_byte(PORT_COM1 + 1, 0x00); // Disable all interrupts
    out_byte(PORT_COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    out_byte(PORT_COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    out_byte(PORT_COM1 + 1, 0x00); //                  (hi byte)
    out_byte(PORT_COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    out_byte(
        PORT_COM1 + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
    out_byte(PORT_COM1 + 4, 0x0b); // IRQs enabled, RTS/DSR set
    out_byte(PORT_COM1 + 4, 0x1e); // Set in loopback mode, test the serial chip
    out_byte(PORT_COM1 + 0, 0xae); // Test serial chip (send byte 0xAE and check if
                               // serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (in_byte(PORT_COM1 + 0) != 0xae) {
        // panic("faulty serial");
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    out_byte(PORT_COM1 + 4, 0x0f);
    return 0;
}

static inline int is_transmit_empty() {
    return in_byte(PORT_COM1 + 5) & 0x20;
}

static inline int serial_received() {
    return in_byte(PORT_COM1 + 5) & 1;
}

char read_serial() {
    while (serial_received() == 0) {}
    return in_byte(PORT_COM1);
}

void write_serial(char a) {
    while (is_transmit_empty() == 0) {}
    out_byte(PORT_COM1, a);
}
