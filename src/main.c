/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <stdio.h>

#include "bsp/board.h"
#include "generated/ws2812.pio.h"

enum {
    COLOR_NOT_MOUNTED = 0x004000,
    COLOR_MOUNTED = 0x000020,
    COLOR_SUSPENDED = 0x00004,
};
#define WS2812_STATE_MACHINE 0
#define WS2812_PIO pio0

_Noreturn void cdc_task(void *params);

#if CFG_TUSB_DEBUG
#define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE)
#else
#define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE/2)
#endif

StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;

// static task for cdc
#define CDC_STACK_SIZE      configMINIMAL_STACK_SIZE
StackType_t cdc_stack[CDC_STACK_SIZE];
StaticTask_t cdc_taskdef;

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

_Noreturn void usb_device_task(void *param) {
    (void) param;

    // This should be called after scheduler/kernel is started
    // otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
    tusb_init();

    // RTOS forever loop
    while (1) {
        // tinyusb device task
        tud_task();
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

/*------------- MAIN -------------*/
int main(void) {
    board_init();
    tusb_init();
    printf("WS2812 Smoke Test, using pin %d", PICO_DEFAULT_WS2812_PIN);
    uint offset = pio_add_program(WS2812_PIO, &ws2812_program);
    ws2812_program_init(WS2812_PIO, WS2812_STATE_MACHINE, offset, PICO_DEFAULT_WS2812_PIN, 800000, false);
    put_pixel(COLOR_NOT_MOUNTED);

    (void) xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack,
                             &usb_device_taskdef);

    (void) xTaskCreateStatic(cdc_task, "cdc", CDC_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef);

    vTaskStartScheduler();

}

#pragma clang diagnostic pop

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    put_pixel(COLOR_MOUNTED);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    put_pixel(COLOR_NOT_MOUNTED);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    put_pixel(COLOR_SUSPENDED);

}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
    put_pixel(COLOR_MOUNTED);
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
_Noreturn void cdc_task(void *params) {
    (void) params;

    // RTOS forever loop
    while (1) {
        // connected() check for DTR bit
        // Most but not all terminal client set this when making connection
        // if ( tud_cdc_connected() )
        {
            // There are data available
            if (tud_cdc_available()) {
                uint8_t buf[64];

                // read and echo back
                uint32_t count = tud_cdc_read(buf, sizeof(buf));
                (void) count;

                // Echo back
                // Note: Skip echo by commenting out write() and write_flush()
                // for throughput test e.g
                //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
                tud_cdc_write(buf, count);
                tud_cdc_write_flush();
            }
        }
    }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) rts;

    // TODO set some indicator
    if (dtr) {
        // Terminal connected
    } else {
        // Terminal disconnected
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
}
