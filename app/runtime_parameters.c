#include "runtime_parameters.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    PARAMETER_PENDULUM_UPRIGHT_ADC = 0,
    PARAMETER_PENDULUM_DIRECTION,
    PARAMETER_TELEMETRY_RATE_HZ,
    PARAMETER_CONTROL_GATE_MAX_SAMPLE_AGE_US,
    PARAMETER_CONTROL_GATE_MAX_ENTRY_THETA_MRAD,
    PARAMETER_CONTROL_ENABLE_REQUEST
} parameter_id_t;

typedef struct {
    runtime_parameter_descriptor_t descriptor;
    parameter_id_t id;
} parameter_entry_t;

static const parameter_entry_t parameter_entries[] = {
    {
        {
            "sensor.pendulum.upright_adc",
            0,
            4095,
            2928,
            "0..4095"
        },
        PARAMETER_PENDULUM_UPRIGHT_ADC
    },
    {
        {
            "sensor.pendulum.direction",
            -1,
            1,
            1,
            "-1|1"
        },
        PARAMETER_PENDULUM_DIRECTION
    },
    {
        {
            "telem.rate_hz",
            1,
            20,
            10,
            "1..20"
        },
        PARAMETER_TELEMETRY_RATE_HZ
    },
    {
        {
            "control.gate.max_sample_age_us",
            500,
            10000,
            2000,
            "500..10000"
        },
        PARAMETER_CONTROL_GATE_MAX_SAMPLE_AGE_US
    },
    {
        {
            "control.gate.max_entry_theta_mrad",
            50,
            1000,
            250,
            "50..1000"
        },
        PARAMETER_CONTROL_GATE_MAX_ENTRY_THETA_MRAD
    },
    {
        {
            "control.enable_request",
            0,
            1,
            0,
            "0|1"
        },
        PARAMETER_CONTROL_ENABLE_REQUEST
    }
};

static const parameter_entry_t *find_parameter(
    const char *name)
{
    size_t index;

    if (name == NULL) {
        return NULL;
    }

    for (index = 0U;
         index < runtime_parameters_count();
         index++) {
        if (strcmp(parameter_entries[index].descriptor.name,
                   name) == 0) {
            return &parameter_entries[index];
        }
    }

    return NULL;
}

static int32_t read_value(
    const runtime_parameters_t *parameters,
    parameter_id_t id)
{
    switch (id) {
    case PARAMETER_PENDULUM_UPRIGHT_ADC:
        return parameters->pendulum_upright_adc;

    case PARAMETER_PENDULUM_DIRECTION:
        return parameters->pendulum_direction;

    case PARAMETER_TELEMETRY_RATE_HZ:
        return parameters->telemetry_rate_hz;

    case PARAMETER_CONTROL_GATE_MAX_SAMPLE_AGE_US:
        return (int32_t)parameters->control_gate_max_sample_age_us;

    case PARAMETER_CONTROL_GATE_MAX_ENTRY_THETA_MRAD:
        return parameters->control_gate_max_entry_theta_mrad;

    case PARAMETER_CONTROL_ENABLE_REQUEST:
        return parameters->control_enable_request ? 1 : 0;

    default:
        return 0;
    }
}

static void write_value(
    runtime_parameters_t *parameters,
    parameter_id_t id,
    int32_t value)
{
    switch (id) {
    case PARAMETER_PENDULUM_UPRIGHT_ADC:
        parameters->pendulum_upright_adc = (uint16_t)value;
        break;

    case PARAMETER_PENDULUM_DIRECTION:
        parameters->pendulum_direction = (int8_t)value;
        break;

    case PARAMETER_TELEMETRY_RATE_HZ:
        parameters->telemetry_rate_hz = (uint8_t)value;
        break;

    case PARAMETER_CONTROL_GATE_MAX_SAMPLE_AGE_US:
        parameters->control_gate_max_sample_age_us = (uint32_t)value;
        break;

    case PARAMETER_CONTROL_GATE_MAX_ENTRY_THETA_MRAD:
        parameters->control_gate_max_entry_theta_mrad = value;
        break;

    case PARAMETER_CONTROL_ENABLE_REQUEST:
        parameters->control_enable_request = (value != 0);
        break;

    default:
        break;
    }
}

void runtime_parameters_init_defaults(
    runtime_parameters_t *parameters)
{
    if (parameters == NULL) {
        return;
    }

    parameters->pendulum_upright_adc = 2928U;
    parameters->pendulum_direction = 1;
    parameters->telemetry_rate_hz = 10U;
    parameters->control_gate_max_sample_age_us = 2000U;
    parameters->control_gate_max_entry_theta_mrad = 250;
    parameters->control_enable_request = false;
    parameters->dirty = false;
}

size_t runtime_parameters_count(void)
{
    return sizeof(parameter_entries) /
           sizeof(parameter_entries[0]);
}

const runtime_parameter_descriptor_t *
runtime_parameters_descriptor(size_t index)
{
    if (index >= runtime_parameters_count()) {
        return NULL;
    }

    return &parameter_entries[index].descriptor;
}

runtime_parameter_result_t runtime_parameters_get(
    const runtime_parameters_t *parameters,
    const char *name,
    int32_t *value)
{
    const parameter_entry_t *entry;

    if ((parameters == NULL) || (value == NULL)) {
        return RUNTIME_PARAMETER_NOT_FOUND;
    }

    entry = find_parameter(name);
    if (entry == NULL) {
        return RUNTIME_PARAMETER_NOT_FOUND;
    }

    *value = read_value(parameters, entry->id);
    return RUNTIME_PARAMETER_OK;
}

runtime_parameter_result_t runtime_parameters_set_text(
    runtime_parameters_t *parameters,
    const char *name,
    const char *value_text)
{
    const parameter_entry_t *entry;
    char *end;
    long parsed;

    if (parameters == NULL) {
        return RUNTIME_PARAMETER_NOT_FOUND;
    }

    entry = find_parameter(name);
    if (entry == NULL) {
        return RUNTIME_PARAMETER_NOT_FOUND;
    }

    if ((value_text == NULL) || (*value_text == '\0')) {
        return RUNTIME_PARAMETER_INVALID_FORMAT;
    }

    errno = 0;
    end = NULL;
    parsed = strtol(value_text, &end, 10);

    if ((errno != 0) ||
        (end == value_text) ||
        (*end != '\0') ||
        (parsed < INT32_MIN) ||
        (parsed > INT32_MAX)) {
        return RUNTIME_PARAMETER_INVALID_FORMAT;
    }

    if ((parsed < entry->descriptor.minimum) ||
        (parsed > entry->descriptor.maximum) ||
        ((entry->id == PARAMETER_PENDULUM_DIRECTION) &&
         (parsed == 0))) {
        return RUNTIME_PARAMETER_OUT_OF_RANGE;
    }

    if (read_value(parameters, entry->id) != (int32_t)parsed) {
        write_value(parameters, entry->id, (int32_t)parsed);
        parameters->dirty = true;
    }

    return RUNTIME_PARAMETER_OK;
}

bool runtime_parameters_is_dirty(
    const runtime_parameters_t *parameters)
{
    return (parameters != NULL) && parameters->dirty;
}
