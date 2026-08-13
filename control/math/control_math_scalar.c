#include "control_math.h"

float control_math_dot_f32(
    const float *a,
    const float *b,
    size_t length)
{
    float sum = 0.0F;
    size_t index;

    if ((a == 0) ||
        (b == 0)) {
        return 0.0F;
    }

    for (index = 0U; index < length; index++) {
        sum += a[index] * b[index];
    }

    return sum;
}

control_math_backend_t control_math_get_backend(void)
{
    return CONTROL_MATH_BACKEND_SCALAR;
}
