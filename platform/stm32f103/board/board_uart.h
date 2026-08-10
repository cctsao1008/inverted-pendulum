#ifndef BOARD_UART_H
#define BOARD_UART_H

#include <stdint.h>

void board_uart_init(uint32_t baudrate);
void board_uart_write(const char *data, uint32_t length);

#endif
