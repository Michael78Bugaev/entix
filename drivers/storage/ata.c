#include <ata.h>
#include <port_based.h>
#include <string.h>
#include <debug.h>
#include <vga.h>

static ata_drive_t drives[4];

static void ata_wait(uint16_t base) {
    uint8_t status; uint32_t timeout=1000000;
    do {
        status = inb(base + ATA_STATUS);
        if(--timeout==0){
            break;
        }
    } while (status & ATA_SR_BSY);
}

static int ata_check_error(uint16_t base) {
    uint8_t status = inb(base + ATA_STATUS);
    if (status & ATA_SR_ERR) return -1;
    return 0;
}

static void ata_select_drive(uint16_t base, uint8_t drive) {
    uint8_t value = 0xE0 | (drive << 4);
    outb(base + ATA_DRIVE, value);
    ata_wait(base);
}

static int ata_init_drive(uint16_t base, uint8_t drive) {
    ata_select_drive(base, drive);
    
    outb(base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    ata_wait(base);
    
    if (ata_check_error(base)) return -1;

    uint16_t buffer[256];
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base + ATA_DATA);
    }

    drives[drive].present = 1;
    drives[drive].type = 1; // ATA
    drives[drive].sectors = *(uint32_t*)&buffer[60];
    drives[drive].size = drives[drive].sectors * 512;

    char* name = (char*)&buffer[27];
    for (int i = 0; i < 20; i++) {
        drives[drive].name[i*2] = name[i*2+1];
        drives[drive].name[i*2+1] = name[i*2];
    }
    drives[drive].name[40] = '\0';
    
    return 0;
}

static void read_device_info(uint16_t base, char* model) {

    outb(base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    ata_wait(base);
    
    if (ata_check_error(base)) {
        strcpy(model, "Unknown");
        return;
    }

    uint16_t buffer[256];
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base + ATA_DATA);
    }

    char* name = (char*)&buffer[27];
    for (int i = 0; i < 20; i++) {
        model[i*2] = name[i*2+1];
        model[i*2+1] = name[i*2];
    }
    model[40] = '\0';

    int len = strlen(model);
    while (len > 0 && model[len-1] == ' ') {
        model[len-1] = '\0';
        len--;
    }
}

static int ata_identify_device(uint16_t base, uint8_t drive, char* model) {
    outb(base + ATA_DRIVE, 0xA0 | (drive << 4));
    inb(base + ATA_STATUS);
    for (int i = 0; i < 1000; i++) {
        if (!(inb(base + ATA_STATUS) & 0x80)) break;
    }
    outb(base + ATA_COMMAND, 0xEC);
    uint8_t status = 0;
    for (int i = 0; i < 1000; i++) {
        status = inb(base + ATA_STATUS);
        if (!(status & 0x80) && (status & 0x08)) break;
    }
    if (!(status & 0x08)) return -1;

    uint16_t buffer[256];
    for (int i = 0; i < 256; i++) buffer[i] = inw(base + ATA_DATA);

    for (int i = 0; i < 20; i++) {
        model[i*2] = buffer[27+i] >> 8;
        model[i*2+1] = buffer[27+i] & 0xFF;
    }
    model[40] = 0;
    for (int i = 39; i >= 0 && model[i] == ' '; i--) model[i] = 0;
    return 0;
}

void ata_init() {
    memset(drives, 0, sizeof(drives));
    
    if (ata_init_drive(ATA_PRIMARY_BASE, 0) == 0) {
        drives[0].present = 1;
        kdbg(KINFO, "ata: found primary master\n");
    }
    
    if (ata_init_drive(ATA_PRIMARY_BASE, 1) == 0) {
        drives[1].present = 1;
        kdbg(KINFO, "ata: found primary slave\n");
    }
    
    if (ata_init_drive(ATA_SECONDARY_BASE, 0) == 0) {
        drives[2].present = 1;
        kdbg(KINFO, "ata: found secondary master\n");
    }
    
    if (ata_init_drive(ATA_SECONDARY_BASE, 1) == 0) {
        drives[3].present = 1;
        kdbg(KINFO, "ata: found secondary slave\n");
    }
}

int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t* buffer) {
    if (drive >= 4 || !drives[drive].present) return -1;
    uint16_t base = (drive < 2) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    drive = drive % 2;

    ata_select_drive(base, drive);

    outb(base + ATA_SECTOR_COUNT, 1);
    outb(base + ATA_SECTOR_NUM, lba & 0xFF);
    outb(base + ATA_CYL_LOW, (lba >> 8) & 0xFF);
    outb(base + ATA_CYL_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));

    outb(base + ATA_COMMAND, ATA_CMD_READ);
    ata_wait(base);

    if (ata_check_error(base)) return -1;

    __asm__ volatile (
        "rep insw"
        : "+D"(buffer)
        : "d"(base + ATA_DATA), "c"(256)
        : "memory"
    );

    return 0;
}

int ata_write_sector(uint8_t drive, uint32_t lba, uint8_t* buffer) {
    if (drive >= 4 || !drives[drive].present) return -1;
    uint16_t base = (drive < 2) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    uint8_t head = drive % 2;
    ata_select_drive(base, head);
    outb(base + ATA_SECTOR_COUNT, 1);
    outb(base + ATA_SECTOR_NUM, lba & 0xFF);
    outb(base + ATA_CYL_LOW, (lba >> 8) & 0xFF);
    outb(base + ATA_CYL_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_DRIVE, 0xE0 | (head << 4) | ((lba >> 24) & 0x0F));
    outb(base + ATA_COMMAND, ATA_CMD_WRITE);
    uint8_t status;
    uint32_t timeout = 1000000;

    do { status = inb(base + ATA_STATUS); } while (((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) && --timeout);

    if (timeout == 0 || (status & ATA_SR_ERR)) {
        return -1;
    }
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outw(base + ATA_DATA, data);
    }
    timeout = 1000000;

    do { status = inb(base + ATA_STATUS); } while ((status & ATA_SR_BSY) && --timeout);
    if (timeout == 0 || (status & ATA_SR_ERR)) {
        return -1;
    }
    return 0;
}

ata_drive_t* ata_get_drive(uint8_t drive) {
    if (drive >= 4 || !drives[drive].present) return NULL;
    return &drives[drive];
} 