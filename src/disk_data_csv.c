#include "main.h"

#define CSV_HEADER "ID    ,lat           ,lon           ,ele  ,time                ,kmh  ,head,PmBar ,barAlt,sat,_"
#define CSV_HEADER_LEN (sizeof CSV_HEADER - 1)
//                  999999,-060.363525012,-128.610957683,12000,2022-07-17T22:58:43Z,123.1,272 ,1056.1,91911.1,19,

#define CSV_LINES_PER_SECT (DISK_SECT_SIZE / sizeof CSV_HEADER) //CR counted instead of \0

static unsigned int writeUInt(uint8_t *buffer, unsigned int val, int digits);

static unsigned int writeCoord(uint8_t *buffer, uint64_t coord, bool positive);

static uint writeDataLine(MEASUREMENT *record, unsigned int idx, uint8_t *buffer) {
    *buffer = '\n';
    uint strIdx = 1 + writeUInt(buffer + 1, idx, 6);
    strIdx += writeCoord(buffer + strIdx, record->position.latDeg_x_1000000000, record->position.latNorth);
    strIdx += writeCoord(buffer + strIdx, record->position.lonDeg_x_1000000000, record->position.lonEast);
    strIdx += writeUInt(buffer + strIdx, record->position.altM, 5);
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.year2020 + 2020, 4);
    buffer[strIdx++] = '-';
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.month, 2);
    buffer[strIdx++] = '-';
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.day, 2);
    buffer[strIdx++] = 'T';
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.hour, 2);
    buffer[strIdx++] = ':';
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.min, 2);
    buffer[strIdx++] = ':';
    strIdx += uIntToCharArray(buffer + strIdx, record->dateTime.sec, 2);
    buffer[strIdx++] = 'Z';
    buffer[strIdx++] = ',';

    strIdx += uIntToCharArray(buffer + strIdx, record->move.speedKmH_x_10 / 10, 3);
    buffer[strIdx++] = '.';
    strIdx += uIntToCharArray(buffer + strIdx, record->move.speedKmH_x_10 % 10, 1);
    buffer[strIdx++] = ',';
    strIdx += uIntToCharArray(buffer + strIdx, record->move.headDegree, 3);
    buffer[strIdx++] = ',';
    strIdx += uIntToCharArray(buffer + strIdx, record->pressureKPa_x_10 / 10, 4);
    buffer[strIdx++] = '.';
    strIdx += uIntToCharArray(buffer + strIdx, record->pressureKPa_x_10 % 10, 1);
    buffer[strIdx++] = ',';
    //todo barAlt
    buffer[strIdx++] = '3';
    buffer[strIdx++] = '1';
    buffer[strIdx++] = '4';
    buffer[strIdx++] = '5';
    buffer[strIdx++] = '8';
    buffer[strIdx++] = '.';
    buffer[strIdx++] = '0';
    buffer[strIdx++] = ',';
    strIdx += uIntToCharArray(buffer + strIdx, record->satInView, 2);
    buffer[strIdx++] = ',';
    return strIdx;
}

static unsigned int writeUInt(uint8_t *buffer, unsigned int val, int digits) {
    unsigned int strIdx = uIntToCharArray(buffer, val, digits);
    buffer[strIdx++] = ',';
    return strIdx;
}

static unsigned int writeCoord(uint8_t *buffer, uint64_t coord, bool positive) {
    unsigned int idx = 0;
    if (!positive) buffer[idx++] = '-';
    idx += uIntToCharArray(buffer + idx, coord / DEG_DIVIDER, 3);
    buffer[idx++] = '.';
    idx += uI64ToCharArray(buffer + idx, coord % DEG_DIVIDER, 9);
    buffer[idx++] = ',';
    return idx;
}


unsigned int csvFileClusters() {
    return (csvFileLength() + DISK_CLUSTER_SIZE - 1) / DISK_CLUSTER_SIZE;
}

unsigned long csvFileLength() {
    if (getMeasurementCnt() < CSV_LINES_PER_SECT) {
        return CSV_HEADER_LEN + getMeasurementCnt() * (CSV_HEADER_LEN + 1);
    } else {
        int sectorsNoTail = (int) ((getMeasurementCnt() + 1) / CSV_LINES_PER_SECT);
        int trailingSectorLines = (int) ((getMeasurementCnt() + 1) % CSV_LINES_PER_SECT);
        return sectorsNoTail * DISK_SECT_SIZE + trailingSectorLines * (CSV_HEADER_LEN + 1);
    }
}

void csvFillDataSector(unsigned int dataLba, uint8_t *ptr, int bytesLeft) {
    int lineCnt = MIN(CSV_LINES_PER_SECT, bytesLeft + ((bytesLeft + sizeof CSV_HEADER - 1) / sizeof CSV_HEADER));
    unsigned int flashRecordIndex;
    if (dataLba == 0) {
        flashRecordIndex = 0;
        memcpy(ptr, CSV_HEADER, CSV_HEADER_LEN);
        ptr += CSV_HEADER_LEN;
        bytesLeft -= CSV_HEADER_LEN;
        lineCnt--;
    } else {
        flashRecordIndex = dataLba * CSV_LINES_PER_SECT - 1;
    }
    lineCnt = MIN(lineCnt, (int) getMeasurementCnt() - flashRecordIndex);
    for (; (lineCnt > 0) && (bytesLeft > sizeof CSV_HEADER); --lineCnt, ++flashRecordIndex) {
        MEASUREMENT *record = getMeasurement(flashRecordIndex);
        if (record->signature != EMPTY) {
            int written = (int) writeDataLine(record, flashRecordIndex, ptr);
            ptr += written;
            bytesLeft -= written;
        }
    }
    if (bytesLeft > 0) {
        memset(ptr, 32, bytesLeft);
    }
}
