#include <stdlib.h>
#include "hardware/flash.h"
#include "tusb.h"
#include "main.h"

#include "disk_data.inc"

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
    char *ptr = buffer;
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
            case (RESERVED_SECTORS + FAT_SECTORS * 2): /* Root directory */
                memcpy(ptr, fake_dir, sizeof fake_dir);
                break;
            default:
            {
                int dataBlock = (int)lba - RESERVED_SECTORS - 2*FAT_SECTORS - ROOT_SECTORS;
                if(dataBlock>=0) {
                    memset(ptr, ' ', bufSize);
                    itoa((int) dataBlock, ptr, 10);
                    ptr[strlen(ptr)] = '!';
                }
            }

        }
        lba++;
        bytesLeft-=bytesToRead;
//    uint32_t addr = XIP_BASE + FLASH_RESERVED_SIZE + lba * DISK_BLOCK_SIZE;
//    memcpy(buffer, (void *) addr, bufsize);
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
