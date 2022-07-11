# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "D:/work/tests/rp2040/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2040-data-logger C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(rp2040-data-logger rp2040-data-logger.c )

pico_set_program_name(rp2040-data-logger "rp2040-data-logger")
pico_set_program_version(rp2040-data-logger "0.1")

pico_enable_stdio_uart(rp2040-data-logger 1)
pico_enable_stdio_usb(rp2040-data-logger 0)

# Add the standard library to the build
target_link_libraries(rp2040-data-logger pico_stdlib)

# Add any user requested libraries
target_link_libraries(rp2040-data-logger
        hardware_pio
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )

pico_add_extra_outputs(rp2040-data-logger)

