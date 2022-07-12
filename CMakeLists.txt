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

add_executable(rp2040-data-logger
        src/main.c
        src/msc_disk.c
        src/usb_descriptors.c
        src/tusb_config.h
        ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/family.c
        )

pico_set_program_name(rp2040-data-logger "rp2040-data-logger")
pico_set_program_version(rp2040-data-logger "0.1")


# Add the standard library to the build
target_link_libraries(rp2040-data-logger pico_stdlib)

## TinyUSB support
target_include_directories(rp2040-data-logger PRIVATE
        ${PICO_SDK_PATH}/lib/tinyusb/src
        ${PICO_SDK_PATH}/lib/tinyusb/hw
        )
target_compile_definitions(rp2040-data-logger PRIVATE
        CFG_TUSB_MCU=OPT_MCU_RP2040
        CFG_TUSB_OS=OPT_OS_PICO#[[TODO OPT_OS_FREERTOS]]
        )
target_sources(rp2040-data-logger PRIVATE
        ${PICO_SDK_PATH}/lib/tinyusb/src/tusb.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/common/tusb_fifo.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/device/usbd.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/device/usbd_control.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/class/cdc/cdc_device.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/class/msc/msc_device.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c
        ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/rp2040/rp2040_usb.c
        )
## End of TinyUSB support

# Add any user requested libraries
target_link_libraries(rp2040-data-logger
        hardware_pio
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )

target_include_directories(rp2040-data-logger PRIVATE src)
pico_add_extra_outputs(rp2040-data-logger)

