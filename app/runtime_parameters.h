#ifndef RUNTIME_PARAMETERS_H
#define RUNTIME_PARAMETERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t pendulum_upright_adc;
    int8_t pendulum_direction;
    uint8_t telemetry_rate_hz;
    bool dirty;
} runtime_parameters_t;

typedef struct {
    const char *name;
    int32_t minimum;
    int32_t maximum;
    int32_t default_value;
    const char *allowed_values;
} runtime_parameter_descriptor_t;

typedef enum {
    RUNTIME_PARAMETER_OK = 0,
    RUNTIME_PARAMETER_NOT_FOUND,
    RUNTIME_PARAMETER_INVALID_FORMAT,
    RUNTIME_PARAMETER_OUT_OF_RANGE
} runtime_parameter_result_t;

void runtime_parameters_init_defaults(
    runtime_parameters_t *parameters);

size_t runtime_parameters_count(void);

const runtime_parameter_descriptor_t *
runtime_parameters_descriptor(size_t index);

runtime_parameter_result_t runtime_parameters_get(
    const runtime_parameters_t *parameters,
    const char *name,
    int32_t *value);

runtime_parameter_result_t runtime_parameters_set_text(
    runtime_parameters_t *parameters,
    const char *name,
    const char *value_text);

bool runtime_parameters_is_dirty(
    const runtime_parameters_t *parameters);

#endif
