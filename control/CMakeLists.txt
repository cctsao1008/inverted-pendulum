add_library(control_core STATIC
    control_config.c
    safety_manager.c
    state_estimator.c
    balance_controller.c

    control_types.c
    control_runtime.c
    sensor_acquisition.c
    kalman_estimator.c
    estimator_dispatch.c
    state_safety.c
    control_state_machine.c
    energy_swing_up.c
    capture_controller.c
    lqr_controller.c
    lqi_controller.c
    controller_dispatch.c
    actuator_mapper.c
    output_safety.c
    motor_output.c
    control_trace.c
    control_pipeline.c

    math/control_math_scalar.c
)

target_include_directories(control_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/math
)

target_compile_options(control_core PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)
