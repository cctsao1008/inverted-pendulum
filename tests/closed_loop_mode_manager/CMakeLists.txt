add_executable(test_closed_loop_mode_manager
    test_closed_loop_mode_manager.c
    ${PROJECT_SOURCE_DIR}/app/closed_loop_mode_manager.c
)

target_include_directories(test_closed_loop_mode_manager PRIVATE
    ${PROJECT_SOURCE_DIR}/app
    ${PROJECT_SOURCE_DIR}/control
)

target_link_libraries(test_closed_loop_mode_manager PRIVATE
    control_core
)

target_compile_options(test_closed_loop_mode_manager PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)

add_test(
    NAME closed_loop_mode_manager
    COMMAND test_closed_loop_mode_manager
)
