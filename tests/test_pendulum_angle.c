#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "pendulum_angle.h"

static void test_upright_is_zero(void)
{
    assert(pendulum_angle_wrapped_counts(2928U, 2928U, 1) == 0);
    assert(pendulum_angle_radians(2928U, 2928U, 1) == 0.0F);
}

static void test_adc_wrap_uses_shortest_delta(void)
{
    assert(pendulum_angle_wrapped_counts(10U, 4090U, 1) == 16);
    assert(pendulum_angle_wrapped_counts(4090U, 10U, 1) == -16);
}

static void test_direction_can_be_inverted(void)
{
    assert(pendulum_angle_wrapped_counts(10U, 4090U, -1) == -16);
    assert(pendulum_angle_wrapped_counts(4090U, 10U, -1) == 16);
}

static void test_half_turn_has_canonical_negative_value(void)
{
    assert(pendulum_angle_wrapped_counts(2048U, 0U, 1) == -2048);
    assert(pendulum_angle_wrapped_counts(2048U, 0U, -1) == -2048);
}

static void test_radians_are_in_expected_range(void)
{
    float angle = pendulum_angle_radians(3028U, 2928U, 1);

    assert(fabsf(angle - 0.153398F) < 0.00001F);
}

int main(void)
{
    test_upright_is_zero();
    test_adc_wrap_uses_shortest_delta();
    test_direction_can_be_inverted();
    test_half_turn_has_canonical_negative_value();
    test_radians_are_in_expected_range();

    printf("PASS: pendulum angle tests\n");
    return 0;
}
