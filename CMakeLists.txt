# What CMake to start at
cmake_minimum_required(VERSION 3.12)
 
# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)
 
# Set the name and version of the project
project(PhaserStrobe VERSION 1.0.0)
 
# Link the Project to a source file (step 4.6)
add_executable(PhaserStrobe source.c modbus_rtu.c flash.c)
 
# Link the Project to an extra library (pico_stdlib)
target_link_libraries(PhaserStrobe pico_stdlib pico_multicore hardware_adc)
 
# Initalise the SDK
pico_sdk_init()
 
# Enable USB, UART output
pico_enable_stdio_usb(PhaserStrobe 1)
pico_enable_stdio_uart(PhaserStrobe 0)
 
# Enable extra outputs (SWD?)
pico_add_extra_outputs(PhaserStrobe)

