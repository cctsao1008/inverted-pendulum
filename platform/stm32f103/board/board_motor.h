#ifndef BOARD_MOTOR_H
#define BOARD_MOTOR_H

#include <stdint.h>

void board_motor_init(void);
void board_motor_set_percent(int8_t signed_percent);
void board_motor_stop(void);

#endif
