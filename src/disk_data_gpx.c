#include "main.h"

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

unsigned int gpxFileClustersNoTail() {
    return (gpxFileSectorsNoTail() + DISK_SECT_PER_CLUSTER - 1) / DISK_SECT_PER_CLUSTER;
}

unsigned long gpxFileLength() {
    return gpxFileSectorsNoTail() * DISK_SECT_SIZE + GPX_TAIL_LEN;
}

//todo test corner data cases
void gpxFillDataSector(unsigned int dataLba, uint8_t *ptr, size_t bytesLeft) {
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
        //todo restarted tracks
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
