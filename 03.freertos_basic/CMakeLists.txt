cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(pico_console C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include(FreeRTOS_Kernel_import.cmake)

add_executable(pico_console
    main.c
    freertos_basic.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_KERNEL_PATH}/timers.c 
)

target_include_directories(pico_console PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_console
    pico_stdlib
    FreeRTOS-Kernel
)

pico_add_extra_outputs(pico_console)
