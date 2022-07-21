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
        unsigned int year2020: 7;
        unsigned int month: 4;
        unsigned int day: 5;
        unsigned int hour: 5;
        unsigned int min: 6;
        unsigned int sec: 6;
    } dateTime;
    struct __attribute__((__packed__)) {
        bool latNorth: 1;
        unsigned long long latDeg_x_1000000000: 40;
        bool lonEast: 1;
        unsigned long long lonDeg_x_1000000000: 40;
        int altM: 17;
    } position;
    struct __attribute__((__packed__)) {
        unsigned int speedKmH_x_10: 16;
        int headDegree: 9;
    } move;
    unsigned int satInView: 6;
    unsigned int pressureKPa_x_10: 15;
} MEASUREMENT;
#define DEG_DIVIDER (1000000000LL)
#define MEASUREMENT_SIZE (32)
#define FLASH_RESERVED_SIZE (0x20000)

#define MAX_MEASUREMENT_RECORDS ((PICO_FLASH_SIZE_BYTES - FLASH_RESERVED_SIZE) / MEASUREMENT_SIZE)
static_assert(sizeof(MEASUREMENT) <= MEASUREMENT_SIZE, "MEASUREMENT struct size");

MEASUREMENT *getMeasurement(size_t idx);

unsigned int getMeasurementCnt();

void resetMeasurementCnt();//todo when written

enum {
    DISK_SIZE_MB = 256,
    DISK_SIZE = DISK_SIZE_MB * 1024 * 1024,
    DISK_SECT_SIZE = CFG_TUD_MSC_EP_BUFSIZE,
    DISK_SECT_NUM = DISK_SIZE / DISK_SECT_SIZE,
    DISK_SECT_NUM_SHORT = (DISK_SECT_NUM > 65535) ? 0 : DISK_SECT_NUM,
    DISK_SECT_NUM_LARGE = (DISK_SECT_NUM <= 65535) ? 0 : DISK_SECT_NUM,
    DISK_CLUSTER_SIZE = 32 * 1024, // 32KB
    DISK_SECT_PER_CLUSTER = DISK_CLUSTER_SIZE / DISK_SECT_SIZE,
    DISK_FAT_BYTES = (DISK_SIZE / DISK_CLUSTER_SIZE) * 2,
    DISK_FAT_SECTORS = MAX((DISK_FAT_BYTES + DISK_SECT_SIZE - 1) / DISK_SECT_SIZE, DISK_SECT_PER_CLUSTER),
    DISK_RESERVED_SECTORS = DISK_SECT_PER_CLUSTER,
    DISK_ROOT_SECTORS = DISK_SECT_PER_CLUSTER,
    DISK_ROOT_ENTRIES = DISK_ROOT_SECTORS * DISK_SECT_SIZE / 32,
    DISK_GPX_FIRST_DATA_SECT = DISK_RESERVED_SECTORS + 2 * DISK_FAT_SECTORS + DISK_ROOT_SECTORS,
    DISK_CSV_FIRST_DATA_CLUSTER = 2 + (DISK_SECT_NUM - DISK_GPX_FIRST_DATA_SECT) /DISK_SECT_PER_CLUSTER /2,
    DISK_CSV_FIRST_DATA_SECT = (DISK_CSV_FIRST_DATA_CLUSTER - 2) * DISK_SECT_PER_CLUSTER + DISK_GPX_FIRST_DATA_SECT
};

void fillRootDirectoryData(uint8_t *buffer, size_t bytesLeft);

void fillBootSector(uint8_t *ptr, size_t bytesToRead);

void fillFat(unsigned int fatLba, uint8_t *ptr, size_t bytesLeft);

unsigned int gpxFileClustersNoTail();

unsigned long gpxFileLength();

void gpxFillDataSector(unsigned int dataLba, uint8_t *ptr, size_t bytesLeft);

unsigned int csvFileClusters();

unsigned long csvFileLength();

unsigned int uIntToCharArray(uint8_t *string, unsigned int value, unsigned int digits);
unsigned int uI64ToCharArray(uint8_t *string, uint64_t value, unsigned int digits);

void csvFillDataSector(unsigned int dataLba, uint8_t *ptr, int bytesLeft);

#define WORD_TO_BYTES(w) ((unsigned char) (w)), ((unsigned char) (((unsigned int) (w)) >> 8))
#define DWORD_TO_BYTES(d) ((unsigned char) (d)), ((unsigned char) (((unsigned int) (d)) >> 8)), \
                            ((unsigned char) (((unsigned int) (d)) >> 16)), ((unsigned char) (((unsigned int) (d)) >> 24))

#endif //RP2040_DATA_LOGGER_MAIN_H
