#include "main.h"

#define WORD_TO_BYTES(w) ((unsigned char) (w)), ((unsigned char) (((unsigned int) (w)) >> 8))
#define DWORD_TO_BYTES(d) ((unsigned char) (d)), ((unsigned char) (((unsigned int) (d)) >> 8)), \
                            ((unsigned char) (((unsigned int) (d)) >> 16)), ((unsigned char) (((unsigned int) (d)) >> 24))

const char GPX_HEAD[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
                        "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"MapSource 6.16.3\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
                        "  <trk>\n"
                        "    <name>Track DD-MMM-YYYY HH:MM:SS</name>\n"
                        "    <trkseg>\n";
const char GPX_TRK_TAIL[] = "    </trkseg>\n"
                            "  </trk>\n"
                            "</gpx>";
//todo test high north west
const char GPX_TRK_RECORD[] = "\n      <trkpt lat=\"-60.36352501250\" lon=\"-128.61095768399\">\n"
                              "        <ele>12000</ele>\n"
                              "        <time>2022-07-17T22:58:43Z</time>\n"
                              "        <cmt>mBar: 1056.11</cmt>\n"
                              "      </trkpt>";
const char GPX_TRK_FMT[] = "\n      <trkpt lat=\"%02d.%09lld\" lon=\"%03d.%09lld\">\n"
                           "        <ele>%5d</ele>\n"
                           "        <time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n"
                           "        <cmt>mBar: %4d.%02d</cmt>\n"
                           "      </trkpt>";

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
    DWORD_TO_BYTES(10000), // 10KiB todo update
};


void intToCharArray(uint8_t *string, unsigned int value, unsigned int digits) {
    do {
        --digits;
        string[digits] = '0' + value % 10;
        value /= 10;
    } while (digits > 0);
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
    intToCharArray(fileRecordPtr, recordPtr->dateTime.day, 2);
    fileRecordPtr[3] = MONTH_NAMES[recordPtr->dateTime.month - 1][0];
    fileRecordPtr[4] = MONTH_NAMES[recordPtr->dateTime.month - 1][1];
    intToCharArray(fileRecordPtr + 2 + 4, recordPtr->dateTime.year2020 + 20, 2);
}

enum {
    GPX_HEAD_LEN = sizeof GPX_HEAD - 1,
    GPX_TAIL_LEN = sizeof GPX_TRK_TAIL - 1,
    GPX_RECORD_LEN = sizeof GPX_TRK_RECORD - 1,
    GPX_RECORD_PER_SECT = (DISK_SECT_SIZE + GPX_RECORD_LEN - 1) / GPX_RECORD_LEN,
    GPX_RECORD_PER_FIRST_SECT = (DISK_SECT_SIZE - GPX_HEAD_LEN + GPX_RECORD_LEN - 1) / GPX_RECORD_LEN
};


static unsigned int gpxFileSectorsNoTail() {
    int cnt = (int) getMeasurementCnt();
    return 1 + (MAX(0, (cnt - GPX_RECORD_PER_FIRST_SECT)) + GPX_RECORD_PER_SECT - 1) / GPX_RECORD_PER_SECT;
}

static unsigned int gpxFileClustersNoTail() {
    return (gpxFileSectorsNoTail() + DISK_SECT_PER_CLUSTER - 1) / DISK_SECT_PER_CLUSTER;
}

static unsigned long gpxFileLength() {
    return gpxFileSectorsNoTail() * DISK_SECT_SIZE + GPX_TAIL_LEN;
}

void fillRootDirectoryData(uint8_t *buffer, size_t bytesLeft) {
    unsigned int emptyRecIdx = getMeasurementCnt();
    if (emptyRecIdx == 0) { //No data, no data file
        memcpy(buffer, fake_dir, 32 * 2); // volume label + LEFT_100.% file
        memset(buffer + 32 * 3, 0, bytesLeft - 32 * 2);
    } else {
        memcpy(buffer, fake_dir, 32 * 3); // volume label + LEFT_100.% file + data file
        memset(buffer + 32 * 3, 0, bytesLeft - 32 * 3);
        MEASUREMENT *lastRec = getMeasurement(emptyRecIdx - 1);
        unsigned int percent = 100 - emptyRecIdx * 100 / MAX_MEASUREMENT_RECORDS;

        uint16_t dateWord =
            ((lastRec->dateTime.year2020 + 40) << 9) | (lastRec->dateTime.month << 5) | lastRec->dateTime.day;
        uint16_t timeWord =
            (lastRec->dateTime.hour << 11) | (lastRec->dateTime.min << 5) | (lastRec->dateTime.sec >> 1);
        //Indicator file name
        intToCharArray(buffer + 32 + 5, percent, 3);
        updateFileDate(buffer + 32, dateWord, timeWord);
        //Data file name
        updateFileName(buffer + 32 * 2, lastRec);
        updateFileDate(buffer + 32 * 2, dateWord, timeWord);
        unsigned long length = gpxFileLength();
        for (int i = 0; i < 8; i++) {
            buffer[32 * 2 + 28 + i] = (uint8_t) length;
            length = length >> 8;
        }
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
    unsigned int lastCluster = gpxFileClustersNoTail() + 3;
    while (bytesLeft > 0) {
        if (currentCluster < lastCluster) {
            currentCluster++;
            *(ptr++) = (uint8_t) currentCluster;
            *(ptr++) = currentCluster >> 8;
        } else if (currentCluster == lastCluster) {
            currentCluster++;
            *(ptr++) = 0xFF;
            *(ptr++) = 0xFF;
        } else {
            *(ptr++) = 0;
            *(ptr++) = 0;
        }
        bytesLeft -= 2;
    }
    asm("nop");
}
//todo test corner data cases
void fillDataSector(unsigned int dataLba, uint8_t *ptr, size_t bytesLeft) {
    memset(ptr, 32, bytesLeft);
    int flashRecordIndex;
    int gpxRecordsLeft;
    unsigned int recCount = getMeasurementCnt();
    if (dataLba == 0) {
        flashRecordIndex = 0;
        memcpy(ptr, GPX_HEAD, sizeof GPX_HEAD - 1);
        ptr += sizeof GPX_HEAD - 1;
        bytesLeft -= sizeof GPX_HEAD - 1;
        gpxRecordsLeft = MIN(GPX_RECORD_PER_FIRST_SECT, recCount - flashRecordIndex);
        //todo start time
    } else if(dataLba == gpxFileSectorsNoTail()) {
        memcpy(ptr, GPX_TRK_TAIL, sizeof GPX_TRK_TAIL - 1);
        gpxRecordsLeft = 0;
    } else {
        flashRecordIndex = GPX_RECORD_PER_FIRST_SECT + ((int)dataLba - 1) * GPX_RECORD_PER_SECT;
        gpxRecordsLeft = MIN(GPX_RECORD_PER_SECT, recCount - flashRecordIndex);
    }
    for (; gpxRecordsLeft > 0; gpxRecordsLeft--) {
        MEASUREMENT *record = getMeasurement(flashRecordIndex);
        int latDeg = (int) ((record->position.latNorth ? 1LL : -1LL) * record->position.latDeg_x_1000000000 /
                            DEG_DIVIDER);
        int lonDeg = (int) ((record->position.lonEast ? 1LL : -1LL) * record->position.lonDeg_x_1000000000 /
                            DEG_DIVIDER);
        int writtenChars =
            sniprintf((char *) ptr, bytesLeft, GPX_TRK_FMT,
                      latDeg,
                      record->position.latDeg_x_1000000000 % DEG_DIVIDER,

                      lonDeg,
                      record->position.lonDeg_x_1000000000 % DEG_DIVIDER,

                      record->position.altM,
                      (record->dateTime.year2020 + 2020), record->dateTime.month, record->dateTime.day,
                      record->dateTime.hour, record->dateTime.min, record->dateTime.sec,
                      record->pressureKPa_x_10 / 10, record->pressureKPa_x_10 % 10);
        if (writtenChars < 0) return;
        ptr += writtenChars;
        *ptr = ' ';
        bytesLeft -= writtenChars;
        flashRecordIndex++;
    }
}
