add_library(control_core STATIC
    control_config.c
    safety_manager.c
)

target_include_directories(control_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(control_core PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wundef
)
