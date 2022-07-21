#include "main.h"
unsigned char BOOT_SECTOR_HEAD[0x27] = {
    0xEB, 0x3C, 0x90, //Jump instruction
    'E', 'L', 'M', 'O', 'T', '2', '.', '0', //OEM NAME
    WORD_TO_BYTES(DISK_SECT_SIZE), // 512 Bytes per Sector
    DISK_SECT_PER_CLUSTER, // Sectors Per Cluster
    WORD_TO_BYTES(DISK_RESERVED_SECTORS), // Reserved Sectors
    0x02, // 2 FATs
    WORD_TO_BYTES(DISK_ROOT_ENTRIES), //  Root Entries
    WORD_TO_BYTES(DISK_SECT_NUM_SHORT), // More than 2^16 sectors on disk
    0xF8, // Media Type - super floppy
    WORD_TO_BYTES(DISK_FAT_SECTORS), //	Sectors per FAT
    0x40, 0x00,             // 64 Sectors per track
    0x20, 0x00,             // 32 Heads
    DWORD_TO_BYTES(0), // No Hidden Sectors
    DWORD_TO_BYTES(DISK_SECT_NUM_LARGE),    // Sectors.
    0x80,                   // Physical Disk Number is 0x80.
    0x00,                   // Current Head. Not used by the FAT file system.
    0x28                    // Signature. Must be either 0x28 or 0x29 in order to be recognized by Windows NT.
};

static const char fake_dir[] = {
    'E', 'L', 'M', 'O', 'T', ' ', 'L', 'O', 'G', ' ', '2', //NAME
    0x28,// Volume label, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES((21 << 11) | (40 << 5) | (4 >> 1)), // 21:40.04
    WORD_TO_BYTES(((2022 - 1980) << 9) | (8 << 5) | 1), // 1-Aug-2022
    WORD_TO_BYTES(0), // Empty file
    DWORD_TO_BYTES(0), // Empty file

    'L', 'E', 'F', 'T', '_', '1', '0', '0', '%', ' ', ' ', //NAME
    0x20,// Normal File, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES((21 << 11) | (40 << 5) | (4 >> 1)), // time 21:40.04
    WORD_TO_BYTES(((2022 - 1980) << 9) | (8 << 5) | 1), // date 1-Aug-2022

    WORD_TO_BYTES(0), // Empty file
    DWORD_TO_BYTES(0), // Empty file

    '0', '0', '-', 'X', 'X', '-', '0', '0', 'G', 'P', 'X', //NAME
    0x20,// Normal File, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES(0), // time
    WORD_TO_BYTES(0), // date
    WORD_TO_BYTES(2), //starting cluster #2
    DWORD_TO_BYTES(10000), // 10KiB

    '0', '0', '-', 'X', 'X', '-', '0', '0', 'C', 'S', 'V', //NAME
    0x20,// Normal File, archive
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //reserved
    WORD_TO_BYTES(0), // time
    WORD_TO_BYTES(0), // date
    WORD_TO_BYTES(DISK_CSV_FIRST_DATA_CLUSTER), //starting cluster #2
    DWORD_TO_BYTES(10000), // 10KiB
};


void updateFileLength(uint8_t *lengthBuffer, unsigned long length);

unsigned int longLongToCharArray(uint8_t *string, uint64_t value, unsigned int digits) {
    unsigned int cnt = digits;
    do {
        --cnt;
        string[cnt] = '0' + value % 10;
        value /= 10;
    } while (cnt > 0);
    return digits;
}

unsigned int uI64ToCharArray(uint8_t *string, uint64_t value, unsigned int digits) {
    unsigned int cnt = digits;
    do {
        --cnt;
        string[cnt] = '0' + value % 10;
        value /= 10;
    } while (cnt > 0);
    return digits;
}

unsigned int uIntToCharArray(uint8_t *string, unsigned int value, unsigned int digits) {
    unsigned int cnt = digits;
    do {
        --cnt;
        string[cnt] = '0' + value % 10;
        value /= 10;
    } while (cnt > 0);
    return digits;
}

void fillBootSector(uint8_t *ptr, size_t bytesToRead) {
    memset(ptr, 0, bytesToRead);
    memcpy(ptr, BOOT_SECTOR_HEAD, MIN(sizeof BOOT_SECTOR_HEAD, bytesToRead));
    if (bytesToRead >= 511) {
        ptr[510] = 0x55;
    }
    if (bytesToRead >= 512) {
        ptr[511] = 0xAA;
    }
}

static inline void updateFileDate(uint8_t *fileRecordPtr, uint16_t dateWord, uint16_t timeWord) {
    fileRecordPtr[22] = timeWord;
    fileRecordPtr[23] = timeWord >> 8;
    fileRecordPtr[24] = dateWord;
    fileRecordPtr[25] = dateWord >> 8;
}

static inline void updateFileName(uint8_t *fileRecordPtr, MEASUREMENT *recordPtr) {
    static const char MONTH_NAMES[12][2] = {
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
    uIntToCharArray(fileRecordPtr, recordPtr->dateTime.day, 2);
    fileRecordPtr[3] = MONTH_NAMES[recordPtr->dateTime.month - 1][0];
    fileRecordPtr[4] = MONTH_NAMES[recordPtr->dateTime.month - 1][1];
    uIntToCharArray(fileRecordPtr + 2 + 4, recordPtr->dateTime.year2020 + 20, 2);
}

void fillRootDirectoryData(uint8_t *buffer, size_t bytesLeft) {
    unsigned int emptyRecIdx = getMeasurementCnt();
    if (emptyRecIdx == 0) { //No data, no data file
        memcpy(buffer, fake_dir, 32 * 2); // volume label + LEFT_100.% file
        memset(buffer + 32 * 2, 0, bytesLeft - 32 * 2);
    } else {
        memcpy(buffer, fake_dir, 32 * 4); // volume label + LEFT_100.% file + 2 data files
        memset(buffer + 32 * 4, 0, bytesLeft - 32 * 4);
        MEASUREMENT *lastRec = getMeasurement(emptyRecIdx - 1);
        unsigned int percent = 100 - emptyRecIdx * 100 / MAX_MEASUREMENT_RECORDS;

        uint16_t dateWord =
            ((lastRec->dateTime.year2020 + 40) << 9) | (lastRec->dateTime.month << 5) | lastRec->dateTime.day;
        uint16_t timeWord =
            (lastRec->dateTime.hour << 11) | (lastRec->dateTime.min << 5) | (lastRec->dateTime.sec >> 1);
        //Indicator file name
        uIntToCharArray(buffer + 32 + 5, percent, 3);
        updateFileDate(buffer + 32, dateWord, timeWord);
        //GPX File
        updateFileName(buffer + 32 * 2, lastRec);
        updateFileDate(buffer + 32 * 2, dateWord, timeWord);
        updateFileLength(buffer + 32 * 2, gpxFileLength());
        //CSV File
        updateFileName(buffer + 32 * 3, lastRec);
        updateFileDate(buffer + 32 * 3, dateWord, timeWord);
        updateFileLength(buffer + 32 * 3, csvFileLength());
    }

}

void updateFileLength(uint8_t *lengthBuffer, unsigned long length) {
    for (int i = 0; i < 4; i++) {
        lengthBuffer[28 + i] = (uint8_t) length;
        length = length >> 8;
    }
}

void fillFat(unsigned int fatLba, uint8_t *buffer, size_t bytesLeft) {
    uint8_t *ptr = buffer;
    uint16_t currentCluster;
    if (fatLba == 0) {
        currentCluster = 2;
        *(ptr++) = 0xF8;
        *(ptr++) = 0xFF;
        *(ptr++) = 0xFF;
        *(ptr++) = 0xFF;
        bytesLeft -= 4;
    } else {
        currentCluster = fatLba * DISK_SECT_SIZE / 2;
    }
    unsigned int gpsLastCluster = gpxFileClustersNoTail() + 3;
    unsigned int csvLastCluster = DISK_CSV_FIRST_DATA_CLUSTER + csvFileClusters();
    while (bytesLeft > 0) {
        if ((currentCluster < gpsLastCluster)
        ||(currentCluster < csvLastCluster) && (currentCluster>=DISK_CSV_FIRST_DATA_CLUSTER))
        {
            currentCluster++;
            *(ptr++) = (uint8_t) currentCluster;
            *(ptr++) = currentCluster >> 8;
        } else if ((currentCluster == gpsLastCluster) || (currentCluster == csvLastCluster)) {
            currentCluster++;
            *(ptr++) = 0xFF;
            *(ptr++) = 0xFF;
        } else {
            *(ptr++) = 0;
            *(ptr++) = 0;
        }
        bytesLeft -= 2;
    }
}
