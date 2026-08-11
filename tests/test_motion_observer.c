#include <assert.h>
#include <stdio.h>

#include "motion_observer.h"

static void test_wrap_safe_position_accumulation(void)
{
    motion_observer_t observer;

    motion_observer_init(&observer, 32766);
    motion_observer_update_1ms(&observer, 32767);
    motion_observer_update_1ms(&observer, -32768);
    motion_observer_update_1ms(&observer, -32767);
    assert(observer.position_counts == 3);
}

static void test_sliding_velocity_windows(void)
{
    motion_observer_t observer;
    int32_t count = 0;
    uint32_t tick;

    motion_observer_init(&observer, count);
    for (tick = 0U; tick < 40U; tick++) {
        count += 4;
        motion_observer_update_1ms(&observer, count);
    }

    assert(observer.position_counts == 160);
    assert(observer.fast_velocity_valid);
    assert(observer.slow_velocity_valid);
    assert(observer.fast_velocity_counts_s == 4000);
    assert(observer.slow_velocity_counts_s == 4000);
}

int main(void)
{
    test_wrap_safe_position_accumulation();
    test_sliding_velocity_windows();
    printf("PASS: motion observer tests\n");
    return 0;
}
