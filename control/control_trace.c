#include "control_trace.h"

void control_trace_init(
    control_trace_t *trace)
{
    if (trace == 0) {
        return;
    }

    trace->sink = 0;
    trace->context = 0;
}

void control_trace_set_sink(
    control_trace_t *trace,
    control_trace_sink_fn sink,
    void *context)
{
    if (trace == 0) {
        return;
    }

    trace->sink = sink;
    trace->context = context;
}

void control_trace_push(
    const control_trace_t *trace,
    const control_trace_record_t *record)
{
    if ((trace == 0) ||
        (record == 0) ||
        (trace->sink == 0)) {
        return;
    }

    trace->sink(
        record,
        trace->context);
}
