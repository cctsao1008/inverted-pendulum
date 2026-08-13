#include <assert.h>
#include <stddef.h>

#include "control_pipeline.h"

typedef struct {
    unsigned int apply_count;
    unsigned int safe_off_count;
} motor_probe_t;

static void test_motor_apply(
    const actuator_command_t *command,
    void *context)
{
    motor_probe_t *probe =
        (motor_probe_t *)context;

    assert(command != NULL);
    assert(probe != NULL);

    probe->apply_count++;
}

static void test_motor_safe_off(void *context)
{
    motor_probe_t *probe =
        (motor_probe_t *)context;

    assert(probe != NULL);

    probe->safe_off_count++;
}

int main(void)
{
    control_pipeline_t pipeline;
    motor_probe_t probe = {0U, 0U};

    control_pipeline_init(&pipeline);

    control_pipeline_set_motor_output(
        &pipeline,
        test_motor_apply,
        test_motor_safe_off,
        &probe);

    control_pipeline_step(&pipeline);

    assert(
        control_pipeline_get_mode(&pipeline) ==
        CONTROL_MODE_DISABLED);

    assert(probe.apply_count == 0U);
    assert(probe.safe_off_count >= 2U);

    return 0;
}
