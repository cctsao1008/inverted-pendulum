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
