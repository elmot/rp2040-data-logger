#include <stdlib.h>
#include "hardware/flash.h"
#include "tusb.h"
#include "main.h"

volatile static int measurementCnt = -1;
//todo remove fake data

static MEASUREMENT __fake_measurement = {
    .signature = FIX,
    .dateTime = {.year2020 = 2, .month = 7, .day = 14, .hour = 15, .min = 16, .sec = 17},
    .position = {.latNorth = 1, .latDeg_x_1000000000 = (uint64_t) (1000000000.0 * (60 + 22.98765 / 60.0)),
        .lonEast = 1, .lonDeg_x_1000000000 = (uint64_t) (1000000000.0 * (173 + 24.43231 / 60.0)), .altM = 13987},
    .move = {.speedKmH_x_10 = 1542, .headDegree = -30},
    .pressureKPa_x_10 = 9985,
    .satInView = 7
};

static MEASUREMENT __empty = {
    .signature = -1,
    .dateTime = {.year2020 = -1, .month = -1, .day = -1, .hour = -1, .min = -1, .sec = -1},
    .position = {.latNorth = -1, .latDeg_x_1000000000 = -1,
        .lonEast = -1, .lonDeg_x_1000000000 = -1},
    .move = {.speedKmH_x_10 = -1, .headDegree = -1},
    .pressureKPa_x_10 = -1,
    .satInView = -1
};


//end of fake data


MEASUREMENT *getMeasurement(size_t idx) {
    // return ((MEASUREMENT*) XIP_BASE + FLASH_RESERVED_SIZE) + idx;todo production code
    __fake_measurement.position.latDeg_x_1000000000 = 60 * DEG_DIVIDER + idx * 1000000;
    __fake_measurement.dateTime.hour = idx / 3600;
    __fake_measurement.dateTime.min = (idx / 60) % 60;
    __fake_measurement.dateTime.sec = idx % 60;
    return (idx < 20000) ? &__fake_measurement : &__empty;
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

    *block_count = DISK_SECT_NUM;
    *block_size = DISK_SECT_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufSize) {
    if (lun != 0) return -1;
    if (offset != 0) return -1;
    disk_status(DISK_STATUS_UP, true);
    uint8_t *ptr = buffer;
    size_t bytesLeft = bufSize;

    while (bytesLeft > 0) {
        if (lba >= DISK_SECT_NUM) return -1;
        size_t bytesToRead = MIN(bytesLeft, DISK_SECT_SIZE);
        if (lba == 0) {
            fillBootSector(ptr, bytesToRead);
        } else if (lba < DISK_RESERVED_SECTORS) {
            memset(ptr, 0, bytesToRead);
        } else if (lba < DISK_RESERVED_SECTORS + DISK_FAT_SECTORS) {
            fillFat(lba - DISK_RESERVED_SECTORS, ptr, bytesToRead);
        } else if (lba < DISK_RESERVED_SECTORS + DISK_FAT_SECTORS * 2) {
            fillFat(lba - DISK_RESERVED_SECTORS - DISK_FAT_SECTORS, ptr, bytesToRead);
        } else if (lba == DISK_RESERVED_SECTORS + DISK_FAT_SECTORS * 2) {
            fillRootDirectoryData(ptr, bytesToRead);
        } else if ((lba >= DISK_GPX_FIRST_DATA_SECT) && (lba < DISK_CSV_FIRST_DATA_SECT)) {
            gpxFillDataSector(lba - DISK_GPX_FIRST_DATA_SECT, ptr, bytesToRead);
        } else if ((lba >= DISK_CSV_FIRST_DATA_SECT)) {
            csvFillDataSector(lba - DISK_CSV_FIRST_DATA_SECT, ptr, bytesToRead);
        } else {
            memset(buffer, 0x00, bytesLeft);
        }
        lba++;
        bytesLeft -= bytesToRead;
    }
    return (long) bufSize;
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
