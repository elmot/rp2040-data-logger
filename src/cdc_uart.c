#include "main.h"
#include "hardware/uart.h"
#include "task.h"

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_BUF_LEN 64


// static task for cdc
#define CDC_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t cdc_stack[CDC_STACK_SIZE];
StaticTask_t cdc_taskdef;

_Noreturn void cdc_task(void *params);

void serial_init() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    (void) xTaskCreateStatic(cdc_task, "cdc", CDC_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef);
}

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
        static uint8_t buffer[UART_BUF_LEN];
        {
            // Drop incoming USB data
            if (tud_cdc_available()) {
                tud_cdc_read_flush();
            }
            size_t bufIdx = 0;
            for (; uart_is_readable(UART_ID) && (bufIdx < UART_BUF_LEN); ++bufIdx) {
                buffer[bufIdx] = uart_get_hw(UART_ID)->dr;
            }
            if (bufIdx > 0) {
                tud_cdc_write(buffer, bufIdx);
                tud_cdc_write_flush();
            }
        }
    }
}

