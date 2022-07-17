#include <stdlib.h>
#include "hardware/flash.h"
#include "tusb.h"
#include "main.h"

enum {
    DISK_SIZE_MB = 256,
    DISK_SIZE = DISK_SIZE_MB * 1024 * 1024,
    DISK_BLOCK_SIZE = CFG_TUD_MSC_EP_BUFSIZE,
    DISK_BLOCK_NUM = DISK_SIZE / DISK_BLOCK_SIZE,
    DISK_BLOCK_NUM_SHORT = (DISK_BLOCK_NUM > 65535) ? 0 : DISK_BLOCK_NUM,
    DISK_BLOCK_NUM_LARGE = (DISK_BLOCK_NUM <= 65535) ? 0 : DISK_BLOCK_NUM,
    CLUSTER_SIZE = 32 * 1024, // 32KB
    SECTOR_PER_CLUSTER = CLUSTER_SIZE / DISK_BLOCK_SIZE,
    FAT_BYTES = (DISK_SIZE / CLUSTER_SIZE) * 2,
    FAT_SECTORS = MAX((FAT_BYTES + DISK_BLOCK_SIZE - 1) / DISK_BLOCK_SIZE, SECTOR_PER_CLUSTER),
    RESERVED_SECTORS = SECTOR_PER_CLUSTER,
    ROOT_SECTORS = SECTOR_PER_CLUSTER,
    ROOT_ENTRIES = ROOT_SECTORS * DISK_BLOCK_SIZE / 32
};
#define WORD_TO_BYTES(w) ((unsigned char) (w)), ((unsigned char) (((unsigned int) (w)) >> 8))
#define DWORD_TO_BYTES(d) ((unsigned char) (d)), ((unsigned char) (((unsigned int) (d)) >> 8)), \
                            ((unsigned char) (((unsigned int) (d)) >> 16)), ((unsigned char) (((unsigned int) (d)) >> 24))

unsigned char BOOT_SECTOR_HEAD[0x27] = {
    0xEB, 0x3C, 0x90, //Jump instruction
    'E', 'L', 'M', 'O', 'T', '2', '.', '0', //OEM NAME
    WORD_TO_BYTES(DISK_BLOCK_SIZE), // 512 Bytes per Sector
    SECTOR_PER_CLUSTER, // Sectors Per Cluster
    WORD_TO_BYTES(RESERVED_SECTORS), // Reserved Sectors
    0x02, // 2 FATs
    WORD_TO_BYTES(ROOT_ENTRIES), //  Root Entries
    WORD_TO_BYTES(DISK_BLOCK_NUM_SHORT), // More than 2^16 sectors on disk
    0xF8, // Media Type - hard disk.
    WORD_TO_BYTES(FAT_SECTORS), //	Sectors per FAT
    0x40, 0x00,             // 64 Sectors per track
    0x20, 0x00,             // 32 Heads
    DWORD_TO_BYTES(0), // No Hidden Sectors
    DWORD_TO_BYTES(DISK_BLOCK_NUM_LARGE),    // Sectors.
    0x80,                   // Physical Disk Number is 0x80.
    0x00,                   // Current Head. Not used by the FAT file system.
    0x28                    // Signature. Must be either 0x28 or 0x29 in order to be recognized by Windows NT.
};

static char MONTH_NAMES[12][2] = {
    {'J', 'A'},
    {'F', 'E'},
    {'M', 'R'},
    {'A', 'P'},
    {'M', 'Y'},
    {'J', 'N'},
    {'J', 'L'},
    {'A', 'U'},
    {'S', 'E'},
    {'O', 'C'},
    {'N', 'V'},
    {'D', 'E'}
};

static const char fake_dir[] = {
    'E', 'L', 'M', 'O', 'T', ' ', 'L', 'O', 'G', ' ', '2', //NAME
    0x28,// Volume label, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES((21 << 11) | (40 << 5) | (4 >> 1)), // 21:40.04
    WORD_TO_BYTES(((2022 - 1980) << 9) | (8 << 5) | 1), // 1-Aug-2022
    WORD_TO_BYTES(0), // Empty file
    DWORD_TO_BYTES(0), // Empty file

    'L', 'E', 'F', 'T', '_', '1', '0', '0', '%', ' ', ' ', //NAME todo update
    0x20,// Normal File, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES((21 << 11) | (40 << 5) | (4 >> 1)), // time 21:40.04
    WORD_TO_BYTES(((2022 - 1980) << 9) | (8 << 5) | 1), // date 1-Aug-2022

    WORD_TO_BYTES(0), // Empty file
    DWORD_TO_BYTES(0), // Empty file


    '0', '0', '-', 'X', 'X', '-', '0', '0', 'G', 'P', 'X', //NAME todo update
    0x20,// Normal File, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES(0), // time
    WORD_TO_BYTES(0), // date
    WORD_TO_BYTES(2), //starting cluster #2
    DWORD_TO_BYTES(0), // 10KiB todo update
};

volatile static int measurementCnt = -1;
//todo remove fake data

static MEASUREMENT __fake_measurement = {
    .signature = FIX,
    .dateTime = {.year2020 = 2, .month = 7, .day = 14, .hour = 15, .min = 16, .sec = 17},
    .position = {.latNorth = 1, .latDeg = 61, .latMin_x_100000 = 2298765,
        .lonEast = 1, .lonDeg = 23, .lonMin_x_100000 = 2443231},
    .move = {.speedKmH_x_10 = 1542, .headDegree = -30},
    .pressureKPa_x_10 = 9985,
    .satInView = 7
};

static MEASUREMENT __empty = {
    .signature = -1,
    .dateTime = {.year2020 = -1, .month = -1, .day = -1, .hour = -1, .min = -1, .sec = -1},
    .position = {.latNorth = -1, .latDeg = -1, .latMin_x_100000 = -1,
        .lonEast = -1, .lonDeg = -1, .lonMin_x_100000 = -1},
    .move = {.speedKmH_x_10 = -1, .headDegree = -1},
    .pressureKPa_x_10 = -1,
    .satInView = -1
};


//end of fake data

void fillRootDirectoryData(uint8_t *buffer);

void intToCharArray(uint8_t *string, unsigned int value, unsigned int digits);

MEASUREMENT *getMeasurement(size_t idx) {
    // return ((MEASUREMENT*) XIP_BASE + FLASH_RESERVED_SIZE) + idx;todo production code
    return (idx < 200000) ? &__fake_measurement : &__empty;
}

unsigned int getMeasurementCnt() {
    if (measurementCnt < 0) {
        for (measurementCnt = 0; (measurementCnt < MAX_MEASUREMENT_RECORDS)
                                 && getMeasurement(measurementCnt)->signature != EMPTY; ++measurementCnt) {}
    }

    return measurementCnt;
}

void resetMeasurementCnt() {//todo when written
    measurementCnt = -1;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;

    const char vid[] = "Elmot";
    const char pid[] = "PosLogger";
    const char rev[] = "2.0";

    memcpy(vendor_id, vid, strlen(vid)); // NOLINT(bugprone-not-null-terminated-result)
    memcpy(product_id, pid, strlen(pid));// NOLINT(bugprone-not-null-terminated-result)
    memcpy(product_rev, rev, strlen(rev));// NOLINT(bugprone-not-null-terminated-result)
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return lun == 0;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void) lun;

    *block_count = DISK_BLOCK_NUM;
    *block_size = DISK_BLOCK_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufSize) {
    if (lun != 0) return -1;
    if (offset != 0) return -1;
    disk_status(DISK_STATUS_UP, true);
    uint8_t *ptr = buffer;
    size_t bytesLeft = bufSize;

    while (bytesLeft > 0) {
        if (lba >= DISK_BLOCK_NUM) return -1;
        size_t bytesToRead = MIN(bytesLeft, DISK_BLOCK_SIZE);
        memset(ptr, 0, bytesToRead);
        switch (lba) {
            case 0: /* Boot sector */
                memcpy(buffer, BOOT_SECTOR_HEAD, MIN(sizeof(BOOT_SECTOR_HEAD), bytesToRead));
                if (bytesLeft >= 511) {
                    ptr[510] = 0x55;
                }
                if (bytesLeft >= 512) {
                    ptr[511] = 0xAA;
                }
                break;
            case RESERVED_SECTORS:
            case RESERVED_SECTORS + FAT_SECTORS: /* FAT */
                ptr[0] = 0xF8; //head entry #1
                ptr[1] = 0xFF;
                ptr[2] = 0xFF; //head entry #2
                ptr[3] = 0xFF;
                ptr[4] = 0xFF; //eof
                ptr[5] = 0xFF;
                break;
            case (RESERVED_SECTORS + FAT_SECTORS * 2):
                fillRootDirectoryData(ptr);
                break;
            default: {
                //todo fake data
                int dataBlock = (int) lba - RESERVED_SECTORS - 2 * FAT_SECTORS - ROOT_SECTORS;
                if (dataBlock >= 0) {
                    memset(ptr, ' ', bufSize);
                    itoa((int) dataBlock, ptr, 10);
                    ptr[strlen(ptr)] = '!';
                }
            }
        }
        lba++;
        bytesLeft -= bytesToRead;
    }
    return (long) bufSize;
}

void fillRootDirectoryData(uint8_t *buffer) {
    unsigned int emptyRecIdx = getMeasurementCnt();
    if (emptyRecIdx == 0) { //No data, no data file
        memcpy(buffer, fake_dir, 32 * 2); // volume label + LEFT_100.% file
        memset(buffer + 32 * 3, 0, 8);
    } else {
        memcpy(buffer, fake_dir, 32 * 3); // volume label + LEFT_100.% file + data file
        memset(buffer + 32 * 3, 0, 8);
        MEASUREMENT *lastRec = getMeasurement(emptyRecIdx - 1);
        unsigned int percent = 100 - emptyRecIdx * 100 / MAX_MEASUREMENT_RECORDS;

        uint16_t dateWord =
            ((lastRec->dateTime.year2020 + 40) << 9) | (lastRec->dateTime.month << 5) | lastRec->dateTime.day;
        uint16_t timeWord =
            (lastRec->dateTime.hour << 11) | (lastRec->dateTime.min << 5) | (lastRec->dateTime.sec >> 1);
        //Indicator file name
        intToCharArray(buffer + 32 + 5, percent, 3);
        //Data file name
        intToCharArray(buffer + 32 * 2, lastRec->dateTime.day, 2);
        buffer[32 * 2 + 3] = MONTH_NAMES[lastRec->dateTime.month - 1][0];
        buffer[32 * 2 + 4] = MONTH_NAMES[lastRec->dateTime.month - 1][1];
        intToCharArray(buffer + 32 * 2 + 2 + 4, lastRec->dateTime.year2020 + 20, 2);
        // Change dates

        buffer[32 + 22] =
        buffer[32 * 2 + 22] = timeWord;
        buffer[32 + 23] =
        buffer[32 * 2 + 23] = timeWord >> 8;
        buffer[32 + 24] =
        buffer[32 * 2 + 24] = dateWord;
        buffer[32 + 25] =
        buffer[32 * 2 + 25] = dateWord >> 8;

    }

}

void intToCharArray(uint8_t *string, unsigned int value, unsigned int digits) {
    do {
        --digits;
        string[digits] = '0' + value % 10;
        value /= 10;
    } while (digits > 0);

}

bool tud_msc_is_writable_cb(uint8_t __unused lun) {
    return false;
}

int32_t tud_msc_write10_cb(uint8_t __unused lun, __unused uint32_t __unused lba, __unused uint32_t offset,
                           __unused uint8_t *buffer, __unused uint32_t bufsize) {
    return -1;// Always Write-protected
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {

    void const *response = NULL;
    int32_t respLen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Host is about to read/write etc ... better not to disconnect disk
            respLen = 0;
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // error -> tinyusb could stall and/or response with failed status
            respLen = -1;
            break;
    }

    if (respLen > bufsize) respLen = bufsize;

    if (response && (respLen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, respLen);
        } else {
            // SCSI output
        }
    }

    return respLen;
}
