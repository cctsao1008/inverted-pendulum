add_executable(test_control_config
    test_control_config.c
)

target_link_libraries(test_control_config PRIVATE
    control_core
)

target_compile_options(test_control_config PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME control_config
    COMMAND test_control_config
)

add_executable(test_safety_manager
    test_safety_manager.c
)

target_link_libraries(test_safety_manager PRIVATE
    control_core
)

target_compile_options(test_safety_manager PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME safety_manager
    COMMAND test_safety_manager
)

add_executable(test_state_estimator
    test_state_estimator.c
)

target_link_libraries(test_state_estimator PRIVATE
    control_core
    m
)

target_compile_options(test_state_estimator PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME state_estimator
    COMMAND test_state_estimator
)

add_executable(test_balance_controller
    test_balance_controller.c
)

target_link_libraries(test_balance_controller PRIVATE
    control_core
    m
)

target_compile_options(test_balance_controller PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME balance_controller
    COMMAND test_balance_controller
)

add_executable(test_telemetry_toggle
    test_telemetry_toggle.c
    ${PROJECT_SOURCE_DIR}/app/telemetry_toggle.c
)

target_include_directories(test_telemetry_toggle PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_telemetry_toggle PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME telemetry_toggle
    COMMAND test_telemetry_toggle
)

add_executable(test_pendulum_angle
    test_pendulum_angle.c
    ${PROJECT_SOURCE_DIR}/app/pendulum_angle.c
)

target_include_directories(test_pendulum_angle PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_pendulum_angle PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME pendulum_angle
    COMMAND test_pendulum_angle
)

add_executable(test_runtime_parameters
    test_runtime_parameters.c
    ${PROJECT_SOURCE_DIR}/app/runtime_parameters.c
)

target_include_directories(test_runtime_parameters PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_runtime_parameters PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME runtime_parameters
    COMMAND test_runtime_parameters
)

add_executable(test_command_service
    test_command_service.c
    ${PROJECT_SOURCE_DIR}/app/command_service.c
    ${PROJECT_SOURCE_DIR}/app/motion_observer.c
    ${PROJECT_SOURCE_DIR}/app/motor_test_service.c
    ${PROJECT_SOURCE_DIR}/app/runtime_parameters.c
    ${PROJECT_SOURCE_DIR}/app/telemetry_toggle.c
)

target_include_directories(test_command_service PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_command_service PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME command_service
    COMMAND test_command_service
)

add_executable(test_motor_test_service
    test_motor_test_service.c
    ${PROJECT_SOURCE_DIR}/app/motion_observer.c
    ${PROJECT_SOURCE_DIR}/app/motor_test_service.c
)

target_include_directories(test_motor_test_service PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_motor_test_service PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME motor_test_service
    COMMAND test_motor_test_service
)

add_executable(test_motion_observer
    test_motion_observer.c
    ${PROJECT_SOURCE_DIR}/app/motion_observer.c
)

target_include_directories(test_motion_observer PRIVATE
    ${PROJECT_SOURCE_DIR}/app
)

target_compile_options(test_motion_observer PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME motion_observer
    COMMAND test_motion_observer
)
