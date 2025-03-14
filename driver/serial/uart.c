#include <driver/serial/uart.h>
#include <driver/rtc/dev_ops.h>
#include <driver/acpi/acpi.h>
#include <driver/acpi/res_types.h>
#include <fs/devfs/devfs.h>
#include <fs/fs.h>
#include <minios/dev.h>
#include <minios/asm.h>
#include <minios/assert.h>

static int uart_com_port_io_addr[4] = {-1, -1, -1, -1};

static bool is_transmit_empty(int index) {
    assert(serial_uart_exists(index));
    return !!(inb(uart_com_port_io_addr[index] + 5) & 0x20);
}

static bool is_data_ready(int index) {
    assert(serial_uart_exists(index));
    return !!(inb(uart_com_port_io_addr[index] + 5) & 0x01);
}

bool serial_uart_exists(int index) {
    assert(index >= 0 && index < 4);
    return uart_com_port_io_addr[index] != -1;
}

uint8_t serial_uart_read(int index) {
    assert(serial_uart_exists(index));
    while (!is_data_ready(index)) {}
    return inb(uart_com_port_io_addr[index]);
}

void serial_uart_write(int index, uint8_t byte) {
    assert(serial_uart_exists(index));
    while (!is_transmit_empty(index)) {}
    outb(uart_com_port_io_addr[index], byte);
}

static int serial_uart_init(int index) {
    assert(serial_uart_exists(index));

    //! disable all interrupts
    outb(uart_com_port_io_addr[index] + 1, 0x00);
    //! enable dlab (set baud rate divisor)
    outb(uart_com_port_io_addr[index] + 3, 0x80);
    //! set divisor to 3 (lo byte) 38400 baud
    outb(uart_com_port_io_addr[index] + 0, 0x03);
    //! (hi byte)
    outb(uart_com_port_io_addr[index] + 1, 0x00);
    //! 8 bits, no parity, one stop bit
    outb(uart_com_port_io_addr[index] + 3, 0x03);
    //! enable fifo, clear them, with 14-byte threshold
    outb(uart_com_port_io_addr[index] + 2, 0xc7);
    //! irqs enabled, rts/dsr set
    outb(uart_com_port_io_addr[index] + 4, 0x0b);
    //! set in loopback mode, test the serial chip
    outb(uart_com_port_io_addr[index] + 4, 0x1e);

    //! test the serial chip by sending a byte and checking if it is received correctly
    const uint8_t test_byte = 0xae;
    serial_uart_write(index, test_byte);
    const uint8_t received_byte = serial_uart_read(index);
    if (received_byte != test_byte) { return -1; }

    //! if serial is not faulty set it in normal operation mode, i.e. not-loopback with IRQs enabled
    //! and OUT#1 and OUT#2 bits enabled
    outb(uart_com_port_io_addr[index] + 4, 0x0f);

    return 0;
}

static int uart_file_read(struct file_desc *file, unsigned int count, char *buf) {
    const int dev = file->fd_dentry->d_inode->i_b_cdev;
    if (DEV_MAJOR(dev) != DEV_CHAR_SERIAL) { return -1; }
    const int index = DEV_MINOR(dev);
    if (!serial_uart_exists(index)) { return -1; }
    for (size_t i = 0; i < count; ++i) { buf[i] = serial_uart_read(index); }
    return count;
}

static int uart_file_write(struct file_desc *file, unsigned int count, const char *buf) {
    const int dev = file->fd_dentry->d_inode->i_b_cdev;
    if (DEV_MAJOR(dev) != DEV_CHAR_SERIAL) { return -1; }
    const int index = DEV_MINOR(dev);
    if (!serial_uart_exists(index)) { return -1; }
    for (size_t i = 0; i < count; ++i) { serial_uart_write(index, buf[i]); }
    return count;
}

static struct file_operations uart_file_ops = {
    .read = uart_file_read,
    .write = uart_file_write,
};

static int serial_uart_probe_impl(lai_nsnode_t *node, lai_state_t *state, void *arg, void *data) {
    UNUSED(arg);

    acpi_resdt_io_port_t *d = data;
    assert(d->addr_base_min == d->addr_base_max);
    const int port = d->addr_base_min;
    for (int i = 0; i < 4; ++i) {
        if (uart_com_port_io_addr[i] == port) { return ACPI_VISIT_SKIP; }
    }

    lai_nsnode_t *prop_node = lai_ns_get_child(node, "_UID");
    assert(prop_node != NULL);

    lai_variable_t var = {};
    lai_var_initialize(&var);
    const int resp = lai_eval(&var, prop_node, state);
    assert(resp == LAI_ERROR_NONE);
    assert(lai_obj_get_type(&var) == LAI_TYPE_INTEGER);
    const int index = (int)var.integer - 1;
    lai_var_finalize(&var);

    assert(index >= 0 && index < 4);
    assert(uart_com_port_io_addr[index] == -1);
    uart_com_port_io_addr[index] = port;
    const int retval = serial_uart_init(index);
    if (retval != 0) {
        uart_com_port_io_addr[index] = -1;
    } else {
        register_device(DEV_MAKE_ID(DEV_CHAR_SERIAL, index), &uart_file_ops);
    }

    return ACPI_VISIT_CONTINUE;
}

int serial_uart_probe() {
    acpi_find_device("PNP0501", serial_uart_probe_impl, NULL);
    for (int i = 0; i < 4; ++i) {
        if (uart_com_port_io_addr[i] != -1) { return 0; }
    }
    return -1;
}
