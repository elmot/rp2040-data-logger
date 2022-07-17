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

typedef struct __attribute__((__packed__)) {
    enum {
        EMPTY = 0xFF, START_SERIE = 10, TIME_ONLY = 20, BAD_FIX = 30, FIX = 40, DIFFFIX = 50
    }
        signature: 8;
    struct __attribute__((__packed__)) {
        unsigned int year: 7;
        unsigned int month: 4;
        unsigned int day: 5;
        unsigned int hour: 5;
        unsigned int min: 6;
        unsigned int sec: 6;
    } dateTime;
    struct __attribute__((__packed__)) {
        unsigned int latNorth: 1;
        unsigned int latDeg: 7;
        unsigned int latMin: 6;
        unsigned int lat100000min: 17;
        unsigned int lonEast: 1;
        unsigned int lonDeg: 8;
        unsigned int lonMin: 6;
        unsigned int lon100000min: 17;
        int altM: 17;
    } position;
    struct __attribute__((__packed__)) {
        int speedKmH: 9;
        unsigned int headDegree: 9;
    } move;
    unsigned int satInView: 6;
    unsigned int pressureKPaQ11_4: 15;
} MEASUREMENT;
#define MEASUREMENT_SIZE (32)
static_assert(sizeof(MEASUREMENT) <=MEASUREMENT_SIZE, "MEASUREMENT struct size");

#endif //RP2040_DATA_LOGGER_MAIN_H
