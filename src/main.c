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
#include "main.h"
//todo gps data collection
//todo baro data collection
//todo verify button erase
//todo ws2812 matrix data indication
//todo altitude data table?
//todo accu voltage control
//todo switch log/disk mode
//todo publish baro data via USB CDC
//todo ublox gps setup for high altitudes
//todo yield calls **
//todo power save?
// - USB sleep
// - FREQ down when USB disconnected?

#define USBD_STACK_SIZE     (4*configMINIMAL_STACK_SIZE)

StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;


_Noreturn void usb_device_task(void *param) {
    (void) param;

    // This should be called after scheduler/kernel is started
    // otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
    tusb_init();

    // RTOS forever loop
    while (1) {
        // tinyusb device task
        tud_task();
        taskYIELD();
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

/*------------- MAIN -------------*/
int main(void) {
    board_init();
    tusb_init();
    indicator_init();
    verify_partly_clean_flash();
    serial_init();
    disk_status(DISK_STATUS_NOINIT, false);
    uart_status(UART_STATUS_NOINIT, false);

    (void) xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack,
                             &usb_device_taskdef);
    vTaskStartScheduler();
}

#pragma clang diagnostic pop

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    disk_status(DISK_STATUS_DOWN, false);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    disk_status(DISK_STATUS_DOWN, false);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    disk_status(DISK_STATUS_DOWN, false);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

