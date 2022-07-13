//
// Created by Ilia.Motornyi on 13/07/2022.
//
#include "main.h"
#include "timers.h"

#define WS2812_STATE_MACHINE 0
#define WS2812_PIO pio0
void indicator_cb(TimerHandle_t);

static StaticTimer_t blinky_tmdef;
static TimerHandle_t blinky_tm;

static inline void put_pixel(uint32_t pixel_grb) {
    while (pio_sm_is_tx_fifo_full(WS2812_PIO, WS2812_STATE_MACHINE)) taskYIELD();
    pio_sm_put(WS2812_PIO, WS2812_STATE_MACHINE, pixel_grb << 8u);
}


void indicator_init() {
    uint offset = pio_add_program(WS2812_PIO, &ws2812_program);
    ws2812_program_init(WS2812_PIO, WS2812_STATE_MACHINE, offset, PICO_DEFAULT_WS2812_PIN, 800000, false);
    blinky_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(50), true, NULL, indicator_cb, &blinky_tmdef);
    xTimerStart(blinky_tm, 0);
}

static volatile enum DISK_STATUS v_disk_status;
static volatile enum UART_STATUS v_uart_status;

static volatile bool v_disk_xchange;
static volatile bool v_uart_xchange;

void disk_status(enum DISK_STATUS status, bool xchange) {
     v_disk_status = status;
     v_disk_xchange = xchange;
}
void uart_status(enum UART_STATUS status, bool xchange) {
    v_uart_status = status;
    v_uart_xchange = xchange;
}

static int v_disk_tick_counter = 0;
static int v_uart_tick_counter = 0;
void indicator_cb(TimerHandle_t xTimer)
{
    (void) xTimer;
    uint32_t grb;
    switch (v_disk_status) {
        case DISK_STATUS_ERASING:
            v_disk_tick_counter = (v_disk_tick_counter + 1) % 10;
            grb = (v_disk_tick_counter < 5 ) ? 0x008000 : 0;
            break;
        case DISK_STATUS_DOWN:
            grb = 0x00200;
            break;
        case DISK_STATUS_UP:
            if(v_disk_xchange) {
                v_disk_xchange = false;
                v_disk_tick_counter = 5;
            }
            if(v_disk_tick_counter >0) {
                v_disk_tick_counter--;
                grb =  0x008000;
            } else {
                grb =  0x001000;
            }
            break;
        case DISK_STATUS_NOINIT:
        default:
            v_disk_tick_counter = (v_disk_tick_counter + 1) % 20;
            grb = (v_disk_tick_counter < 3 ) ? 0x002000 : 0x000200;
            break;

    }
    switch (v_uart_status) {
        case UART_STATUS_UP:
            if(v_uart_xchange) {
                v_uart_xchange = false;
                v_uart_tick_counter = 5;
            }
            if(v_uart_tick_counter >0) {
                v_uart_tick_counter--;
                grb |=  0x800000;
            } else {
                grb |=  0x100000;
            }
            break;
        case UART_STATUS_DOWN:
        case UART_STATUS_NOINIT:
        default:
            break;
    }
    put_pixel(grb);
}