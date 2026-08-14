cmake_minimum_required(VERSION 3.16)

project(inverted_pendulum LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(FW_VERSION "0.1.0-dev"
    CACHE STRING
    "Firmware semantic version")

option(BUILD_STM32_FIRMWARE
       "Build STM32F103 firmware"
       ON)

option(BUILD_HOST_TESTS
       "Build host-side unit tests"
       OFF)

add_subdirectory(control)

if(BUILD_STM32_FIRMWARE)
    add_subdirectory(platform/stm32f103)
endif()

if(BUILD_HOST_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
