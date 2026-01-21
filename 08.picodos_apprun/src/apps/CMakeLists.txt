cmake_minimum_required(VERSION 3.13)

# --- Inline equivalent of toolchain-arm-none-eabi.cmake (must be before project()) ---
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set compilers only if not specified externally
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER arm-none-eabi-gcc)
endif()
if(NOT CMAKE_ASM_COMPILER)
  set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
endif()

project(pico_dos_apps C ASM)

set(CMAKE_C_STANDARD 11)

# Tools (Python)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# objcopy / nm (use arm-none-eabi-*)
find_program(OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(NM      arm-none-eabi-nm      REQUIRED)

# --- HELLO app ---
add_executable(app_hello
  hello/app_hello.c
)

target_include_directories(app_hello PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Cortex-M0+ / Thumb
target_compile_options(app_hello PRIVATE
  -mcpu=cortex-m0plus
  -mthumb
  -ffreestanding
  -fdata-sections
  -ffunction-sections
)

# Fixed-address linking + section GC
target_link_options(app_hello PRIVATE
  -T ${CMAKE_CURRENT_LIST_DIR}/app.ld
  -nostartfiles
  -nostdlib
  -Wl,--gc-sections
  -Wl,-e,app_entry
  -Wl,--undefined=app_entry
)

set_target_properties(app_hello PROPERTIES SUFFIX ".elf")

# Generate PXE (ELF→BIN→PXE via Python)
add_custom_command(TARGET app_hello POST_BUILD
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/tools/mkpxe_from_elf.py
          $<TARGET_FILE:app_hello>
          ${CMAKE_CURRENT_BINARY_DIR}/HELLO.PXE
          arm-none-eabi
  BYPRODUCTS
    ${CMAKE_CURRENT_BINARY_DIR}/HELLO.PXE
  COMMENT "Building HELLO.PXE"
)
