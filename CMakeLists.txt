cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)

# Initialize the SDK
pico_sdk_init()
include_directories("..")
include_directories(".")

add_executable(debug debug.c)

# pull in common dependencies
target_link_libraries(debug pico_stdlib hardware_pio hardware_dma)

# USB UART
pico_enable_stdio_usb(debug 1)
pico_enable_stdio_uart(debug 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(debug)

