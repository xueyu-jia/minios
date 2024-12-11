#ifndef _AHCI_H
#define _AHCI_H

#include "type.h"
#define mk_pci_add(bus, dev, fun, reg) \
    0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg << 2
#define MAXBUS 255
#define MAXDEV 31
#define MAXFUN 7

#define PCI_CONFIG_ADDR 0xcf8
#define PCI_CONFIG_DATA 0Xcfc
#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000

#define SATA_SIG_ATA 0x00000101   // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101    // Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

typedef volatile struct tagHBA_PORT {
    u32 clb;       // 0x00, command list base address, 1K-byte aligned
    u32 clbu;      // 0x04, command list base address upper 32 bits
    u32 fb;        // 0x08, FIS base address, 256-byte aligned
    u32 fbu;       // 0x0C, FIS base address upper 32 bits
    u32 is;        // 0x10, interrupt status
    u32 ie;        // 0x14, interrupt enable
    u32 cmd;       // 0x18, command and status
    u32 rsv0;      // 0x1C, Reserved
    u32 tfd;       // 0x20, task file data
    u32 sig;       // 0x24, signature
    u32 ssts;      // 0x28, SATA status (SCR0:SStatus)
    u32 sctl;      // 0x2C, SATA control (SCR2:SControl)
    u32 serr;      // 0x30, SATA error (SCR1:SError)
    u32 sact;      // 0x34, SATA active (SCR3:SActive)
    u32 ci;        // 0x38, command issue
    u32 sntf;      // 0x3C, SATA notification (SCR4:SNotification)
    u32 fbs;       // 0x40, FIS-based switch control
    u32 rsv1[11];  // 0x44 ~ 0x6F, Reserved
    u32 vendor[4]; // 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct tagHBA_MEM {
    // 0x00 - 0x2B, 通用主机控制寄存器(Generic Host Control)
    u32 cap;     // 0x00, Host capability
    u32 ghc;     // 0x04, Global host control
    u32 is;      // 0x08, Interrupt status
    u32 pi;      // 0x0C, Port implemented
    u32 vs;      // 0x10, Version
    u32 ccc_ctl; // 0x14, Command completion coalescing control
    u32 ccc_pts; // 0x18, Command completion coalescing ports
    u32 em_loc;  // 0x1C, Enclosure management location
    u32 em_ctl;  // 0x20, Enclosure management control
    u32 cap2;    // 0x24, Host capabilities extended
    u32 bohc;    // 0x28, BIOS/OS handoff control and status

    // 0x2C - 0x9F, Reserved
    u8 rsv[0xA0 - 0x2C];

    // 0xA0 - 0xFF, Vendor specific registers
    u8 vendor[0x100 - 0xA0];

    // 0x100 - 0x10FF, Port control registers
    HBA_PORT ports[30]; // max:32 ports
} HBA_MEM;

typedef struct tagHBA_CMD_HEADER {
    // DW0
    u8 cfl : 5;  // Command FIS length in DWORDS, 2 ~ 16
    u8 a : 1;    // ATAPI
    u8 w : 1;    // Write, 1: H2D(主机到设备), 0: D2H(设备到主机)
    u8 p : 1;    // Prefetchable
    u8 r : 1;    // Reset
    u8 b : 1;    // BIST
    u8 c : 1;    // Clear busy upon R_OK
    u8 rsv0 : 1; // Reserved
    u8 pmp : 4;  // Port multiplier port
    u16 prdtl;   // Physical region descriptor table length in entries
    // DW1
    volatile u32 prdbc; // Physical region descriptor byte count transferred
    // DW2, 3
    u32 ctba;  // Command table descriptor base addressa,ligned to a 128-byte
               // cache line
    u32 ctbau; // Command table descriptor base address upper 32 bits
    // DW4 - 7
    u32 rsv1[4]; // Reserved
} HBA_CMD_HEADER;

typedef struct tagHBA_PRDT_ENTRY {
    u32 dba;  // Data base addressThe block must be word aligned
    u32 dbau; // Data base address upper 32 bits, indicated by bit 0 being
              // reserved.

    u32 rsv0; // Reserved

    // DW3
    u32 dbc : 22; // Byte count, 4M max
    u32 rsv1 : 9; // Reserved
    u32 i : 1;    // Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL {
    // 0x00
    u8 cfis[64]; // Command FIS

    // 0x40
    u8 acmd[16]; // ATAPI command, 12 or 16 bytes

    // 0x50
    u8 rsv[48]; // Reserved

    // 0x80
    HBA_PRDT_ENTRY
    prdt_entry[8]; // Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;

typedef enum {
    FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
    FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
    FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
} FIS_TYPE;
typedef struct tagFIS_REG_D2H {
    // DWORD 0
    u8 fis_type; // FIS_TYPE_REG_D2H

    u8 pmport : 4; // Port multiplier
    u8 rsv0 : 2;   // Reserved
    u8 i : 1;      // Interrupt bit
    u8 rsv1 : 1;   // Reserved

    u8 status; // Status register
    u8 error;  // Error register

    // DWORD 1
    u8 lba0;   // LBA low register, 7:0
    u8 lba1;   // LBA mid register, 15:8
    u8 lba2;   // LBA high register, 23:16
    u8 device; // Device register

    // DWORD 2
    u8 lba3; // LBA register, 31:24
    u8 lba4; // LBA register, 39:32
    u8 lba5; // LBA register, 47:40
    u8 rsv2; // Reserved

    // DWORD 3
    u8 countl;  // Count register, 7:0
    u8 counth;  // Count register, 15:8
    u8 rsv3[2]; // Reserved

    // DWORD 4
    u8 rsv4[4]; // Reserved
} FIS_REG_D2H;

typedef struct tagFIS_REG_H2D {
    // DWORD 0
    u8 fis_type; // FIS_TYPE_REG_H2D

    u8 pmport : 4; // Port multiplier
    u8 rsv0 : 3;   // Reserved
    u8 c : 1;      // 1: Command, 0: Control

    u8 command;  // Command register
    u8 featurel; // Feature register, 7:0

    // DWORD 1
    u8 lba0;   // LBA low register, 7:0
    u8 lba1;   // LBA mid register, 15:8
    u8 lba2;   // LBA high register, 23:16
    u8 device; // Device register

    // DWORD 2
    u8 lba3;     // LBA register, 31:24
    u8 lba4;     // LBA register, 39:32
    u8 lba5;     // LBA register, 47:40
    u8 featureh; // Feature register, 15:8

    // DWORD 3
    u8 countl;  // Count register, 7:0
    u8 counth;  // Count register, 15:8
    u8 icc;     // Isochronous command completion
    u8 control; // Control register

    // DWORD 4
    u8 rsv1[4]; // Reserved
} FIS_REG_H2D;

typedef enum {
    ATA_CMD_READ = 0x20,    // LBA24
    ATA_CMD_READ_EX = 0x24, // LBA48,by qianglong 2022.4.25
    ATA_CMD_WRITE = 0x30,
    ATA_CMD_WRITE_EX = 0x34,
    ATA_CMD_IDENTIFY = 0xEC,
    ATA_CMD_READ_DMA = 0xC8,
    ATA_CMD_READ_DMA_EX = 0x25,
    ATA_CMD_WRITE_DMA = 0xCA,
    ATA_CMD_WRITE_DMA_EX = 0x35
} FIS_COMMAND;

// AHCI相关信息
typedef struct tagahci_info {
    struct AHCI_POS_PCI // AHCI在PCI总线上的位置
    {
        u32 bus;
        u32 dev;
        u32 fun;
    } achi_pos_pci;
    u8 is_ahci_mode;
    u32 ABAR;    // AHCI基本内存（物理地址）
    u32 portnum; // 支持的端口数0~31,0表示支持1一个端口
    u32 satadrv_num;
    u32 satadrv_atport
        [4]; // 连接了SATADRIVE的端口号0~31，考虑到实际情况，这里只支持4个硬盘,默认第一个为minios系统盘
    u16 irq_info;
} AHCI_INFO;
#define HBA_PxIS_TFES 0x40000000
#define MAX_AHCI_NUM 4
PUBLIC int ahci_sata_init();
PUBLIC u32 identity_SATA(HBA_PORT *port, u8 *buf);
PUBLIC int find_cmdslot(HBA_PORT *port);
PUBLIC void tf_err_rec(HBA_PORT *port);
int sata_read(int dev, u16 *buf, u64 sect, u32 count);
extern HBA_MEM *HBA;

// 可能存在多个AHCI控制器，为了方便目前只支持1个，这里只用遍历到的第一个。
extern AHCI_INFO ahci_info[MAX_AHCI_NUM];
extern volatile int sata_wait_flag;

#endif /* ACHI_H */
