cmake_minimum_required(VERSION 3.16)

project(inverted_pendulum LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_subdirectory(platform/stm32f103)
