cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico_console
    main.c
    commands.c
    basic.c
    rdisk.c
)

target_link_libraries(pico_console pico_stdlib)

pico_add_extra_outputs(pico_console)
