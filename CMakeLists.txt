# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "D:/work/tests/rp2040/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2040-data-logger C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
set(PICO_BOARD sparkfun_promicro)
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(rp2040-data-logger
        src/data_processing.c
        src/cdc_uart.c
        src/disk_data.c
        src/main.h
        src/freertos_hook.c
        src/indicator.c
        src/main.c
        src/msc_disk.c
        src/tusb_config.h
        src/usb_descriptors.c
        ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/family.c
        )
set_target_properties(rp2040-data-logger
        PROPERTIES
        PICO_TARGET_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/memmap_128k.ld
)

pico_set_program_name(rp2040-data-logger "rp2040-data-logger")
pico_set_program_version(rp2040-data-logger "0.1")
pico_generate_pio_header(rp2040-data-logger ${CMAKE_SOURCE_DIR}/src/ws2812.pio OUTPUT_DIR ${CMAKE_SOURCE_DIR}/src/generated)

# Add the standard library to the build
target_link_libraries(rp2040-data-logger pico_stdlib)

target_compile_definitions(rp2040-data-logger PRIVATE
        CFG_TUSB_MCU=OPT_MCU_RP2040
        CFG_TUSB_OS=OPT_OS_FREERTOS
        )
target_include_directories(rp2040-data-logger PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/generated
        )

include(tinyusb-cdc-msc.cmake)

include(FreeRTOS.cmake)

# Add any user requested libraries
target_link_libraries(rp2040-data-logger
        hardware_clocks
        hardware_flash
        hardware_pio
        hardware_sync
        hardware_timer
        hardware_uart
        hardware_watchdog
        FreeRTOS-RP2040-Kernel
        )
target_link_options(rp2040-data-logger PRIVATE -Wl,--print-memory-usage)
pico_add_extra_outputs(rp2040-data-logger)
