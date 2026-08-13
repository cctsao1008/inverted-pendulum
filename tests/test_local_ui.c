#include <assert.h>
#include <string.h>

#include "local_ui.h"

int main(void)
{
    local_ui_t ui;
    local_ui_snapshot_t snapshot = {0};
    local_ui_frame_t frame;

    local_ui_init(&ui);
    assert(local_ui_get_contrast(&ui) == 0x7FU);

    snapshot.control_trace_valid = true;
    snapshot.control_runtime_ready = true;
    snapshot.control_allowed = true;
    snapshot.control_mode = CONTROL_MODE_IDLE;
    snapshot.estimator_mode = STATE_ESTIMATOR_BASIC;
    snapshot.balance_controller = BALANCE_CONTROLLER_LQR;
    snapshot.vbus_millivolts = 12000U;
    snapshot.motor_channel_d1 = false;
    snapshot.oled_contrast = local_ui_get_contrast(&ui);

    local_ui_render(&ui, &snapshot, &frame);
    assert(strstr(frame.lines[0], "STATUS") != NULL);
    assert(strstr(frame.lines[1], "IDLE") != NULL);
    assert(strstr(frame.lines[4], "12000") != NULL);

    local_ui_next_page(&ui);
    snapshot.theta_mrad = 123;
    snapshot.phi_mrad = -456;
    local_ui_render(&ui, &snapshot, &frame);
    assert(strstr(frame.lines[0], "SENSOR") != NULL);
    assert(strstr(frame.lines[1], "+123") != NULL);
    assert(strstr(frame.lines[3], "-456") != NULL);

    local_ui_previous_page(&ui);
    local_ui_render(&ui, &snapshot, &frame);
    assert(strstr(frame.lines[0], "STATUS") != NULL);

    assert(local_ui_increase_contrast(&ui) > 0x7FU);
    assert(local_ui_decrease_contrast(&ui) == 0x7FU);

    return 0;
}
