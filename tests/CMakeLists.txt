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
