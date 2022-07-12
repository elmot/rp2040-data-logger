target_include_directories(rp2040-data-logger PRIVATE
        ${PICO_SDK_PATH}/lib/tinyusb/src
        ${PICO_SDK_PATH}/lib/tinyusb/hw
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
