enum {
    DISK_SIZE_MB = 256,
    DISK_SIZE = DISK_SIZE_MB * 1024 * 1024,
    FLASH_RESERVED_SIZE = 0x20000,
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
unsigned char BOOT_SECTOR_HEAD [0x27] = {
    0xEB, 0x3C, 0x90, //Jump instruction
    'E','L','M','O','T','2','.','0', //OEM NAME
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
    DWORD_TO_BYTES(DISK_BLOCK_NUM_LARGE),	// Sectors.
    0x80,                   // Physical Disk Number is 0x80.
    0x00,                   // Current Head. Not used by the FAT file system.
    0x28                    // Signature. Must be either 0x28 or 0x29 in order to be recognized by Windows NT.
};

static const char fake_dir[] = {
    'E','L','M','O','T',' ','L','O','G',' ','2', //NAME
    0x28,// Volume label, archive
    0,0,0,0,0,0,0,0,0,0, //reserved
    WORD_TO_BYTES(0b1010110100000010), // 21:40.04
    WORD_TO_BYTES(0b1010010100000010), // 2-Sep-2022
    WORD_TO_BYTES(0), //empty file
    DWORD_TO_BYTES(0), // of zero size

    'L','E','F','T','_','X','X','X','%',' ',' ', //NAME
    0x20,// Normal File, archive
    0,0,0,0,0,0,0,0,0,0, //reserved
    WORD_TO_BYTES(0b1010110100000110), // 21:40.12
    WORD_TO_BYTES(((2022/*year*/ - 1980 )<< 9) | (9<<5) | 3), // 3-Sep-2022
    WORD_TO_BYTES(0), // Empty file
    DWORD_TO_BYTES(0), // Empty file


    'D','A','T','A',' ',' ',' ',' ','G','P','X', //NAME
    0x20,// Normal File, archive
    0,0,0,0,0,0,0,0,0,0, //reserved
    WORD_TO_BYTES(0b1010110100000110), // 21:40.12
    WORD_TO_BYTES(((2022/*year*/ - 1980 )<< 9) | (9<<5) | 3), // 3-Sep-2022
    WORD_TO_BYTES(2), //starting cluster #2
    DWORD_TO_BYTES(10000), // 10KiB

    0,0,0,0,0,0,0,0,0,0 //End-of-root
};
