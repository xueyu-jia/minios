#include <serial.h>
#include <x86.h>

#define PORT_COM1 0x3f8

static inline int is_transmit_empty() {
    return inb(PORT_COM1 + 5) & 0x20;
}

static inline int serial_received() {
    return inb(PORT_COM1 + 5) & 0x01;
}

uint8_t serial_read() {
    while (!serial_received()) {}
    return inb(PORT_COM1);
}

void serial_write(uint8_t byte) {
    while (!is_transmit_empty()) {}
    outb(PORT_COM1, byte);
}

bool serial_init() {
    outb(PORT_COM1 + 1, 0x00); //<! disable all interrupts
    outb(PORT_COM1 + 3, 0x80); //<! enable dlab (set baud rate divisor)
    outb(PORT_COM1 + 0, 0x03); //<! set divisor to 3 (lo byte) 38400 baud
    outb(PORT_COM1 + 1, 0x00); //<! (hi byte)
    outb(PORT_COM1 + 3, 0x03); //<! 8 bits, no parity, one stop bit
    outb(PORT_COM1 + 2, 0xc7); //<! enable fifo, clear them, with 14-byte threshold
    outb(PORT_COM1 + 4, 0x0b); //<! irqs enabled, rts/dsr set
    outb(PORT_COM1 + 4, 0x1e); //<! set in loopback mode, test the serial chip

    //! test the serial chip by sending a byte and checking if it is received correctly
    const uint8_t test_byte = 0xae;
    serial_write(test_byte);
    const uint8_t received_byte = serial_read();
    if (received_byte != test_byte) { return false; }

    //! if serial is not faulty set it in normal operation mode, i.e. not-loopback with IRQs enabled
    //! and OUT#1 and OUT#2 bits enabled
    outb(PORT_COM1 + 4, 0x0f);

    return true;
}
