#include "board_time.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

static volatile uint32_t g_tim2_overflow;
static volatile uint32_t g_control_ticks;

void board_time_init(void)
{
    /*
     * TIM2 clock = 72 MHz because APB1 timer clock is doubled.
     * Prescaler 72 gives a 1 MHz counter: 1 count = 1 us.
     */
    rcc_periph_clock_enable(RCC_TIM2);

    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_prescaler(TIM2, 72U - 1U);
    timer_set_period(TIM2, 0xFFFFU);
    timer_clear_flag(TIM2, TIM_SR_UIF);
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    nvic_set_priority(NVIC_TIM2_IRQ, 0x40U);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    timer_enable_counter(TIM2);

    /*
     * 72 MHz / 72000 = 1 kHz SysTick.
     */
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(72000U - 1U);
    systick_clear();
    systick_interrupt_enable();
    systick_counter_enable();
}

uint32_t board_time_micros(void)
{
    uint32_t high_before;
    uint32_t high_after;
    uint16_t low;
    bool overflow_pending;

    do {
        high_before = g_tim2_overflow;
        low = (uint16_t)timer_get_counter(TIM2);
        overflow_pending = timer_get_flag(TIM2, TIM_SR_UIF);
        high_after = g_tim2_overflow;
    } while (high_before != high_after);

    /*
     * Account for an overflow that occurred before CNT was read but whose
     * interrupt has not run yet.
     */
    if (overflow_pending && (low < 0x8000U)) {
        high_before++;
    }

    return (high_before << 16) | low;
}

uint32_t board_time_ticks(void)
{
    return g_control_ticks;
}

void tim2_isr(void)
{
    if (timer_get_flag(TIM2, TIM_SR_UIF)) {
        timer_clear_flag(TIM2, TIM_SR_UIF);
        g_tim2_overflow++;
    }
}

void sys_tick_handler(void)
{
    g_control_ticks++;
}
