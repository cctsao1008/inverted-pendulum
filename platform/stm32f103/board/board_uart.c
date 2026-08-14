#include "board_uart.h"

#include <stddef.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#define BOARD_UART_TX_BUFFER_SIZE 2048U
#define BOARD_UART_TX_BUFFER_MASK (BOARD_UART_TX_BUFFER_SIZE - 1U)

#if (BOARD_UART_TX_BUFFER_SIZE & BOARD_UART_TX_BUFFER_MASK) != 0
#error "BOARD_UART_TX_BUFFER_SIZE must be a power of two"
#endif

static uint8_t g_tx_buffer[BOARD_UART_TX_BUFFER_SIZE];
static volatile uint16_t g_tx_head;
static volatile uint16_t g_tx_tail;
static volatile uint32_t g_tx_dropped_bytes;

static bool board_uart_tx_enqueue(uint8_t byte)
{
    uint16_t head = g_tx_head;
    uint16_t next =
        (uint16_t)((head + 1U) & BOARD_UART_TX_BUFFER_MASK);

    if (next == g_tx_tail) {
        g_tx_dropped_bytes++;
        return false;
    }

    g_tx_buffer[head] = byte;

    /*
     * The main context is the only producer and USART1 ISR is the only
     * consumer. Publish the byte by advancing head only after the data is
     * stored, then ensure TXE interrupt is enabled.
     */
    g_tx_head = next;
    usart_enable_tx_interrupt(USART1);
    return true;
}

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

    g_tx_head = 0U;
    g_tx_tail = 0U;
    g_tx_dropped_bytes = 0U;

    usart_set_baudrate(USART1, baudrate);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);

    /*
     * Keep UART transport below the real-time tick in interrupt priority.
     * TXE remains disabled until the first byte is queued.
     */
    nvic_set_priority(NVIC_USART1_IRQ, 0xC0U);
    nvic_enable_irq(NVIC_USART1_IRQ);
}

void board_uart_write(const char *data, uint32_t length)
{
    uint32_t index;

    if (data == NULL) {
        return;
    }

    for (index = 0U; index < length; index++) {
        if (data[index] == '\n') {
            (void)board_uart_tx_enqueue((uint8_t)'\r');
        }

        (void)board_uart_tx_enqueue((uint8_t)data[index]);
    }
}

uint32_t board_uart_tx_dropped_bytes(void)
{
    return g_tx_dropped_bytes;
}

void usart1_isr(void)
{
    if (!usart_get_flag(USART1, USART_SR_TXE)) {
        return;
    }

    if (g_tx_tail == g_tx_head) {
        usart_disable_tx_interrupt(USART1);
        return;
    }

    usart_send(USART1, g_tx_buffer[g_tx_tail]);
    g_tx_tail =
        (uint16_t)((g_tx_tail + 1U) & BOARD_UART_TX_BUFFER_MASK);

    if (g_tx_tail == g_tx_head) {
        usart_disable_tx_interrupt(USART1);
    }
}

bool board_uart_try_read_char(char *character)
{
    if ((character == NULL) ||
        !usart_get_flag(USART1, USART_SR_RXNE)) {
        return false;
    }

    *character = (char)usart_recv(USART1);
    return true;
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
