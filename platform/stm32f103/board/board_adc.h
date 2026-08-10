#ifndef BOARD_ADC_H
#define BOARD_ADC_H

#include <stdint.h>

void board_adc_init(void);
uint16_t board_adc_read_pendulum_raw(void);

#endif
