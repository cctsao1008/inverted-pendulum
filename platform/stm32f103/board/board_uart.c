#include "board_uart.h"

#include <stddef.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
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
static volatile uint16_t g_tx_dma_length;
static volatile uint32_t g_tx_dropped_bytes;
static volatile bool g_tx_dma_active;

static void board_uart_tx_dma_kick(void)
{
    uint16_t head;
    uint16_t tail;
    uint16_t length;

    if (g_tx_dma_active) {
        return;
    }

    head = g_tx_head;
    tail = g_tx_tail;
    if (head == tail) {
        return;
    }

    length = (head > tail)
        ? (uint16_t)(head - tail)
        : (uint16_t)(BOARD_UART_TX_BUFFER_SIZE - tail);

    g_tx_dma_length = length;
    g_tx_dma_active = true;

    dma_disable_channel(DMA1, DMA_CHANNEL4);
    dma_set_memory_address(
        DMA1,
        DMA_CHANNEL4,
        (uint32_t)&g_tx_buffer[tail]);
    dma_set_number_of_data(DMA1, DMA_CHANNEL4, length);
    dma_enable_channel(DMA1, DMA_CHANNEL4);
}

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
    g_tx_head = next;
    return true;
}

void board_uart_init(uint32_t baudrate)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_DMA1);

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
    g_tx_dma_length = 0U;
    g_tx_dropped_bytes = 0U;
    g_tx_dma_active = false;

    usart_set_baudrate(USART1, baudrate);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);

    /*
     * STM32F103 USART1_TX is mapped to DMA1 Channel 4.
     * One DMA interrupt is generated per contiguous buffer block instead of
     * one USART TXE interrupt per transmitted byte.
     */
    dma_channel_reset(DMA1, DMA_CHANNEL4);
    dma_set_peripheral_address(
        DMA1,
        DMA_CHANNEL4,
        (uint32_t)&USART_DR(USART1));
    dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_LOW);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

    nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 0xC0U);
    nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);

    usart_enable_tx_dma(USART1);
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

    board_uart_tx_dma_kick();
}

uint32_t board_uart_tx_dropped_bytes(void)
{
    return g_tx_dropped_bytes;
}

void dma1_channel4_isr(void)
{
    if ((DMA_ISR(DMA1) & DMA_ISR_TCIF4) == 0U) {
        return;
    }

    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL4, DMA_TCIF);
    dma_disable_channel(DMA1, DMA_CHANNEL4);

    g_tx_tail = (uint16_t)(
        (g_tx_tail + g_tx_dma_length) & BOARD_UART_TX_BUFFER_MASK);
    g_tx_dma_length = 0U;
    g_tx_dma_active = false;

    board_uart_tx_dma_kick();
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
