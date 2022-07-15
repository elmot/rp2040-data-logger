//
// Created by Ilia.Motornyi on 13/07/2022.
//

#ifndef RP2040_DATA_LOGGER_MAIN_H
#define RP2040_DATA_LOGGER_MAIN_H

#include "bsp/board.h"
#include "generated/ws2812.pio.h"
#include "task.h"

enum DISK_STATUS {
    DISK_STATUS_NOINIT,
    DISK_STATUS_UP,
    DISK_STATUS_DOWN,
    DISK_STATUS_ERASING
};

enum UART_STATUS {
    UART_STATUS_NOINIT,
    UART_STATUS_UP,
    UART_STATUS_DOWN
};

void disk_status(enum DISK_STATUS, bool xchange);
void uart_status(enum UART_STATUS, bool xchange);
void indicator_init(void);
void serial_init();
#endif //RP2040_DATA_LOGGER_MAIN_H
