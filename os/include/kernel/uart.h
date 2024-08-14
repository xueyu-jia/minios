#ifndef UART_H
#define UART_H
#define PORT_COM1       0x3f8
#define SERIAL_BUF_SIZE 256

int  init_simple_serial();
char read_serial();
void write_serial(char a);
#endif