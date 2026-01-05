cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

include(FreeRTOS_Kernel_import.cmake)

add_executable(pico_console
    main.c
    commands.c
    basic.c
    rdisk.c
    exec.c
    util.c
    console.c
    kernel/proc.c
    kernel/vfs.c
    kernel/syscall.c
    apps/hello_unix.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_KERNEL_PATH}/timers.c 
)

target_include_directories(pico_console PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_console
    pico_stdlib
    hardware_uart
    hardware_irq
    FreeRTOS-Kernel
)

# Disable optimizations and include debug symbols for easier debugging
target_compile_options(pico_console PRIVATE -O0 -g)

# Optional: default to Debug build type when not specified
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

pico_add_extra_outputs(pico_console)
