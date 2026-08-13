#include "local_ui.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOCAL_UI_CONTRAST_STEP 16U

static void clear_frame(local_ui_frame_t *frame)
{
    uint32_t row;

    for (row = 0U; row < LOCAL_UI_ROWS; row++) {
        frame->lines[row][0] = '\0';
    }
}

static void write_line(
    local_ui_frame_t *frame,
    uint32_t row,
    const char *format,
    ...)
{
    va_list arguments;

    if ((frame == NULL) ||
        (row >= LOCAL_UI_ROWS) ||
        (format == NULL)) {
        return;
    }

    va_start(arguments, format);
    (void)vsnprintf(
        frame->lines[row],
        LOCAL_UI_LINE_BYTES,
        format,
        arguments);
    va_end(arguments);
}

static const char *mode_name(control_mode_t mode)
{
    switch (mode) {
    case CONTROL_MODE_DISABLED:
        return "DISABLED";
    case CONTROL_MODE_IDLE:
        return "IDLE";
    case CONTROL_MODE_SWING_UP:
        return "SWING";
    case CONTROL_MODE_CAPTURE:
        return "CAPTURE";
    case CONTROL_MODE_BALANCE:
        return "BALANCE";
    case CONTROL_MODE_FAULT:
    default:
        return "FAULT";
    }
}

static const char *estimator_name(state_estimator_mode_t mode)
{
    return (mode == STATE_ESTIMATOR_BASIC) ? "BASIC" : "KALMAN";
}

static const char *balance_name(balance_controller_mode_t mode)
{
    return (mode == BALANCE_CONTROLLER_LQR) ? "LQR" : "LQI";
}

void local_ui_init(local_ui_t *ui)
{
    if (ui == NULL) {
        return;
    }

    ui->page = LOCAL_UI_PAGE_STATUS;
    ui->contrast = 0xEFU;
}

void local_ui_next_page(local_ui_t *ui)
{
    if (ui == NULL) {
        return;
    }

    ui->page = (local_ui_page_t)(
        ((uint32_t)ui->page + 1U) %
        (uint32_t)LOCAL_UI_PAGE_COUNT);
}

void local_ui_previous_page(local_ui_t *ui)
{
    if (ui == NULL) {
        return;
    }

    if (ui->page == LOCAL_UI_PAGE_STATUS) {
        ui->page = (local_ui_page_t)(LOCAL_UI_PAGE_COUNT - 1U);
    } else {
        ui->page = (local_ui_page_t)((uint32_t)ui->page - 1U);
    }
}

uint8_t local_ui_increase_contrast(local_ui_t *ui)
{
    if (ui == NULL) {
        return 0U;
    }

    if (ui->contrast > (uint8_t)(0xFFU - LOCAL_UI_CONTRAST_STEP)) {
        ui->contrast = 0xFFU;
    } else {
        ui->contrast = (uint8_t)(ui->contrast + LOCAL_UI_CONTRAST_STEP);
    }

    return ui->contrast;
}

uint8_t local_ui_decrease_contrast(local_ui_t *ui)
{
    if (ui == NULL) {
        return 0U;
    }

    if (ui->contrast < LOCAL_UI_CONTRAST_STEP) {
        ui->contrast = 0U;
    } else {
        ui->contrast = (uint8_t)(ui->contrast - LOCAL_UI_CONTRAST_STEP);
    }

    return ui->contrast;
}

uint8_t local_ui_get_contrast(const local_ui_t *ui)
{
    return (ui == NULL) ? 0U : ui->contrast;
}

void local_ui_render(
    const local_ui_t *ui,
    const local_ui_snapshot_t *snapshot,
    local_ui_frame_t *frame)
{
    if ((ui == NULL) ||
        (snapshot == NULL) ||
        (frame == NULL)) {
        return;
    }

    clear_frame(frame);

    switch (ui->page) {
    case LOCAL_UI_PAGE_STATUS:
        write_line(frame, 0U, "STATUS 1/5");
        write_line(frame, 1U, "MODE %s",
                   mode_name(snapshot->control_mode));
        write_line(frame, 2U, "CTRL %s",
                   snapshot->control_runtime_ready ? "OBSERVE" : "CONFIG ERR");
        write_line(frame, 3U, "MOTOR %s %s",
                   snapshot->maintenance_output_percent == 0 ? "OFF" : "ACTIVE",
                   snapshot->motor_channel_d1 ? "D1" : "D2");
        write_line(frame, 4U, "VBUS %lu mV",
                   (unsigned long)snapshot->vbus_millivolts);
        write_line(frame, 5U, "TELEM %s",
                   snapshot->telemetry_enabled ? "ON" : "OFF");
        write_line(frame, 6U, "MISS %lu",
                   (unsigned long)snapshot->missed_cycles);
        write_line(frame, 7U, "M:NEXT X:BACK");
        break;

    case LOCAL_UI_PAGE_SENSOR:
        write_line(frame, 0U, "SENSOR 2/5");
        write_line(frame, 1U, "TH %+ld mrad",
                   (long)snapshot->theta_mrad);
        write_line(frame, 2U, "TD %+ld mr/s",
                   (long)snapshot->theta_rate_mrad_s);
        write_line(frame, 3U, "PH %+ld mrad",
                   (long)snapshot->phi_mrad);
        write_line(frame, 4U, "PD %+ld mr/s",
                   (long)snapshot->phi_rate_mrad_s);
        write_line(frame, 5U, "ENC %ld",
                   (long)snapshot->encoder_count);
        write_line(frame, 6U, "AGE %lu us",
                   (unsigned long)snapshot->sample_age_us);
        write_line(frame, 7U, "M:NEXT X:BACK");
        break;

    case LOCAL_UI_PAGE_SAFETY:
        write_line(frame, 0U, "SAFETY 3/5");
        write_line(frame, 1U, "ALLOW %s",
                   snapshot->control_allowed ? "YES" : "NO");
        write_line(frame, 2U, "FAULT %08lX",
                   (unsigned long)snapshot->fault_flags);
        write_line(frame, 3U, "AGE %lu us",
                   (unsigned long)snapshot->sample_age_us);
        write_line(frame, 4U, "MISS %lu",
                   (unsigned long)snapshot->missed_cycles);
        write_line(frame, 5U, "TRACE %s",
                   snapshot->control_trace_valid ? "VALID" : "WAIT");
        write_line(frame, 6U, "MOTOR SINK UNBOUND");
        write_line(frame, 7U, "M:NEXT X:BACK");
        break;

    case LOCAL_UI_PAGE_CONTROL:
        write_line(frame, 0U, "CONTROL 4/5");
        write_line(frame, 1U, "MODE %s",
                   mode_name(snapshot->control_mode));
        write_line(frame, 2U, "EST %s",
                   estimator_name(snapshot->estimator_mode));
        write_line(frame, 3U, "BAL %s",
                   balance_name(snapshot->balance_controller));
        write_line(frame, 4U, "U %+ld/1000",
                   (long)snapshot->control_milli);
        write_line(frame, 5U, "PWM %ld.%ld%%",
                   (long)(snapshot->actuator_tenths_percent / 10),
                   (long)(snapshot->actuator_tenths_percent < 0
                       ? -(snapshot->actuator_tenths_percent % 10)
                       : (snapshot->actuator_tenths_percent % 10)));
        write_line(frame, 6U, "AUTH OBSERVE ONLY");
        write_line(frame, 7U, "M:NEXT X:BACK");
        break;

    case LOCAL_UI_PAGE_MAINTENANCE:
    default:
        write_line(frame, 0U, "MAINT 5/5");
        write_line(frame, 1U, "CHANNEL %s",
                   snapshot->motor_channel_d1 ? "D1" : "D2");
        write_line(frame, 2U, "OUTPUT %+d%%",
                   (int)snapshot->maintenance_output_percent);
        write_line(frame, 3U, "ENC %ld",
                   (long)snapshot->encoder_count);
        write_line(frame, 4U, "TELEM %s",
                   snapshot->telemetry_enabled ? "ON" : "OFF");
        write_line(frame, 5U, "CONTRAST %u",
                   (unsigned int)snapshot->oled_contrast);
        write_line(frame, 6U, "+/- CONTRAST");
        write_line(frame, 7U, "USER:TELEM M:NEXT");
        break;
    }
}
