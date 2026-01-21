cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_console
  src/main.c
  src/dos/dos.c
  src/dos/dos_sys.c
  src/dos/shell.c
  src/dos/cmds_core.c
  src/dos/cmds_fs.c
  src/dos/parse.c
  src/dos/apps_builtin.c
  src/vfs/vfs.c
  src/fs/ramfs.c
  src/util/strutil.c
  src/fs/flash_fs.c
  src/os/svc_handler.c  
  src/pxe/pxe_loader.c
  src/xfer/cobs.c
  src/xfer/xfer_recv.c
  src/dos/autoexec.c
  src/dos/shell_exec.c
  )

target_include_directories(pico_console PRIVATE
  src
  src/dos
  src/fs
  src/vfs
  src/util
)

pico_enable_stdio_uart(pico_console 1)
pico_enable_stdio_usb(pico_console 0)

target_link_libraries(pico_console pico_stdlib)
target_link_libraries(pico_console hardware_flash)

# Disable optimizations and include debug symbols for easier debugging
target_compile_options(pico_console PRIVATE -O0 -g)

# Optional: default to Debug build type when not specified
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

pico_add_extra_outputs(pico_console)
