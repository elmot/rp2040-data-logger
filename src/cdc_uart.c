#include "main.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "task.h"
#include "message_buffer.h"

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_ID uart0
#define UART_IRQ UART0_IRQ
#define BAUD_RATE 9600

#define BUFFER_SIZE 1024
#define MAX_NMEA_MSG_SIZE 100
static uint8_t uartBufferStorage[BUFFER_SIZE];
static StaticMessageBuffer_t uartStaticMsgBuffer;
static MessageBufferHandle_t *uartMsgBuffer;

// static task for cdc
#define CDC_STACK_SIZE configMINIMAL_STACK_SIZE
StackType_t cdc_stack[CDC_STACK_SIZE];
StaticTask_t cdc_taskdef;

_Noreturn void cdc_task(void *params);

void on_uart_rx() {
    static uint8_t buffer[MAX_NMEA_MSG_SIZE];
    static size_t bufIdx = 0;
    while (uart_is_readable(UART_ID)) {
        char ch = uart_get_hw(UART_ID)->dr;
        if (bufIdx > (MAX_NMEA_MSG_SIZE - 2)) {
            ch = 10;
        }
        buffer[bufIdx++] = ch;
        if (ch == 10) {
            xMessageBufferSend(uartMsgBuffer, buffer, bufIdx, 0);
            bufIdx = 0;
        }
    }
}
void serial_init() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    (void) xTaskCreateStatic(cdc_task, "cdc", CDC_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef);
    uartMsgBuffer = xMessageBufferCreateStatic(MAX_NMEA_MSG_SIZE, uartBufferStorage, &uartStaticMsgBuffer);

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

// Invoked when cdc when line state changed e.g. connected/disconnected
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

        // Drop incoming USB data
        if (tud_cdc_available()) {
            tud_cdc_read_flush();
        }
        static uint8_t usbBuffer[MAX_NMEA_MSG_SIZE];
        size_t messageLength = xMessageBufferReceive(uartMsgBuffer, usbBuffer, MAX_NMEA_MSG_SIZE, 0);
        if (messageLength > 0) {
            for (uint8_t *pointer = usbBuffer; messageLength > 0;) {
                uint32_t written = tud_cdc_write(pointer, messageLength);
                messageLength -= written;
                pointer += written;
            }
        }
        tud_cdc_write_flush();
    }
    taskYIELD();
}

