#ifndef CONTROL_MATH_H
#define CONTROL_MATH_H

#include <stddef.h>

typedef enum {
    CONTROL_MATH_BACKEND_SCALAR = 0,
    CONTROL_MATH_BACKEND_CMSIS_F32,
    CONTROL_MATH_BACKEND_CMSIS_Q31
} control_math_backend_t;

float control_math_dot_f32(
    const float *a,
    const float *b,
    size_t length);

control_math_backend_t control_math_get_backend(void);

#endif
