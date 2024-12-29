#include <ahci.h>
#include <disk.h>
#include <fat32.h>
#include <terminal.h>
#include <global.h>
#include <x86.h>

u32 bootPartStartSector;
bool found_sata_dev = false;

static void ide_waitdisk() {
    while ((inb(0x1F7) & 0xC0) != DISK_READY_FLAG) {}
}

static int ide_readsect(void *dst, u32 offset) {
    // wait for disk to be ready
    ide_waitdisk();
    outb(DISK_SECT_COUNT_PORT, 1);                // count = 1读写扇区数量
    outb(DISK_SECT_LBA_0_7_PORT, offset);         // LBA参数的0-7位
    outb(DISK_SECT_LBA_8_15_PORT, offset >> 8);   // LBA参数的8-15位
    outb(DISK_SECT_LBA_16_23_PORT, offset >> 16); // LBA参数的16-23位
    outb(DISK_SECT_LBA_24_31_PORT,
         (offset >> 24) | 0xE0); // LBA模式是24-27位为0-3位，第四位0为主盘，1为从盘
    outb(DISK_SECT_CMD_PORT,
         CMD_READ); // cmd 0x20 - read
                    // sectors状态和命令寄存器，操作时先给命令再读取
    // wait for disk to be ready
    ide_waitdisk();
    // read a sector
    insl(DISK_PORT, dst,
         (SECTSIZE / 4)); // 从port0x1F0读取SECTSIZE/4个双字到dst中
    return TRUE;
}
static int ide_read(void *dst, u32 offset, int count) {
    if (count < 0 || count > 8) {
        lprintf("ide_read para err !\n");
        return FALSE;
    }
    int ret = 1;
    for (int i = 0; i < count; i++) { ret &= ide_readsect(dst + i * SECTSIZE, offset + i); }
    return ret;
}

int readsect(void *dst, u32 offset) {
    return readsects(dst, offset, 1);
}

int readsects(void *dst, u32 offset, u32 count) {
    if (found_sata_dev) {
        return sata_read(0, dst, offset, count);
    } else {
        return ide_read(dst, offset, count);
    }
}
