#include <stdint.h>
#include <stdio.h>

#include "board_clock.h"
#include "board_led.h"
#include "board_uart.h"

static void delay_cycles(uint32_t cycles)
{
    while (cycles > 0U) {
        __asm__ volatile ("nop");
        cycles--;
    }
}

int main(void)
{
    board_clock_init();
    board_led_init();
    board_uart_init(115200U);

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n");
    printf("[BOOT] inverted-pendulum\n");
    printf("[MCU] STM32F103C8T6\n");
    printf("[LIB] libopencm3\n");
    printf("[CLK] system=%lu Hz\n",
           (unsigned long)board_clock_get_hz());
    printf("[UART] USART1 PA9/PA10 115200 8N1\n");
    printf("[APP] main loop started\n");

    while (1) {
        board_led_toggle();
        delay_cycles(8000000U);
    }
}
