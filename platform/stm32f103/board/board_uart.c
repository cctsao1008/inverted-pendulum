#include "board_uart.h"

#include <stddef.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

void board_uart_init(uint32_t baudrate)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        GPIO9);

    gpio_set_mode(
        GPIOA,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_FLOAT,
        GPIO10);

    usart_set_baudrate(USART1, baudrate);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

void board_uart_write(const char *data, uint32_t length)
{
    for (uint32_t index = 0U; index < length; index++) {
        if (data[index] == '\n') {
            usart_send_blocking(USART1, '\r');
        }

        usart_send_blocking(USART1, (uint16_t)data[index]);
    }
}

int _write(int file, char *data, int length)
{
    (void)file;

    if ((data == NULL) || (length <= 0)) {
        return 0;
    }

    board_uart_write(data, (uint32_t)length);
    return length;
}
