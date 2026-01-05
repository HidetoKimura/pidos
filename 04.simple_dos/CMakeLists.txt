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
    exec.c
    util.c
)

pico_enable_stdio_uart(pico_console 1)
pico_enable_stdio_usb(pico_console 0)

target_link_libraries(pico_console pico_stdlib)

# Disable optimizations and include debug symbols for easier debugging
target_compile_options(pico_console PRIVATE -O0 -g)

# Optional: default to Debug build type when not specified
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

pico_add_extra_outputs(pico_console)
