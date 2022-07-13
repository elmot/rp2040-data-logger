#include "main.h"

//
// Created by Ilia.Motornyi on 13/07/2022.
//
// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) rts;

    if (dtr) {
        uart_status(UART_STATUS_UP, false);
    } else {
        uart_status(UART_STATUS_DOWN, false);
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
    uart_status(UART_STATUS_UP, true);
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

