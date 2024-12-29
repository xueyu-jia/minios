#include <ahci.h>
#include <disk.h>
#include <terminal.h>
#include <paging.h>
#include <string.h>
#include <x86.h>
#include <compiler.h>
#include <pci.h>
#include <type.h>
#include <disk.h>

#define PCI_SUBCLS_SATA_CTRL 0x06

volatile int sata_wait_flag = 1;

HBA_MEM *HBA;

AHCI_INFO ahci_info[MAX_AHCI_NUM];

static int check_type(HBA_PORT *port);
static void probe_port(HBA_MEM *abar);
void port_rebase(HBA_PORT *port);

static bool do_pci_ahci_init(pci_device_info_t *info, void *arg) {
    int *total_found = (int *)arg;
    if (*total_found >= MAX_AHCI_NUM) { return false; }

    const bool is_ahci_sata =
        info->class_code == PCI_CLASS_MASS_STORAGE_CTRL && info->subclass == PCI_SUBCLS_SATA_CTRL;
    if (!is_ahci_sata) { return true; }

    const int index = (*total_found)++;
    ahci_info[index].achi_pos_pci.bus = PCI_GET_BUS(info->id);
    ahci_info[index].achi_pos_pci.dev = PCI_GET_DEV(info->id);
    ahci_info[index].achi_pos_pci.fun = PCI_GET_FUNC(info->id);
    ahci_info[index].int_line = pci_io_r8(info->id, PCI_CONFIG_HDR0_INTERRUPT_LINE);
    ahci_info[index].int_pin = pci_io_r8(info->id, PCI_CONFIG_HDR0_INTERRUPT_PIN);

    ahci_info[index].ABAR = pci_io_r32(info->id, PCI_CONFIG_HDR0_BAR5);

    return true;
}

bool ahci_sata_init() {
    memset(ahci_info, 0, sizeof(AHCI_INFO));

    u32 ahci_cnt = 0;
    pci_visit(do_pci_ahci_init, &ahci_cnt);

    if (ahci_cnt == 0) {
        lprintf("error: ahci controller not fonud\n");
        return false;
    }
    lprintf("info: found %d ahci controllers\n", ahci_cnt);

    map_laddr(rcr3(), ahci_info[0].ABAR, ahci_info[0].ABAR, PG_P | PG_USS | PG_RWW);

    HBA = u2ptr(ahci_info[0].ABAR);

    ahci_info[0].is_ahci_mode = HBA->ghc >> 31;
    if (!ahci_info[0].is_ahci_mode) {
        lprintf("error: ahci mode not supported");
        return false;
    }

    ahci_info[0].portnum = HBA->cap & 0xff;
    lprintf("info: probe sata devices\n", ahci_cnt);
    probe_port(HBA);

    for (size_t i = 0; i < ahci_info[0].satadrv_num; ++i) {
        port_rebase(&HBA->ports[ahci_info[0].satadrv_atport[i]]);
    }
    found_sata_dev = ahci_info[0].satadrv_num > 0;

    return true;
}

static void probe_port(HBA_MEM *abar) {
    const u32 pi = abar->pi;
    ahci_info[0].satadrv_num = 0;
    for (int i = 0; i < 32; ++i) {
        if ((pi & (1 << i)) == 0) { continue; }
        const int type = check_type(&abar->ports[i]);
        switch (type) {
            case AHCI_DEV_SATA: {
                lprintf("AHCI[port=%02d]: SATA Drive\n", i);
                ahci_info[0].satadrv_atport[ahci_info[0].satadrv_num++] = i;
            } break;
            case AHCI_DEV_SATAPI: {
                lprintf("AHCI[port=%02d]: SATAPI Drive\n", i);
            } break;
            case AHCI_DEV_SEMB: {
                lprintf("AHCI[port=%02d]: Enclosure Management Bridge\n", i);
            } break;
            case AHCI_DEV_PM: {
                lprintf("AHCI[port=%02d]: Port Multiplier\n", i);
            } break;
        }
    }
}

// Check device type
static int check_type(HBA_PORT *port) {
    u32 ssts = port->ssts;

    u8 ipm = (ssts >> 8) & 0x0F;
    u8 det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT) // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NULL;

    switch (port->sig) {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
    }
}

void start_cmd(HBA_PORT *port) {
    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void stop_cmd(HBA_PORT *port) {
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (1) {
        if (port->cmd & HBA_PxCMD_FR) continue;
        if (port->cmd & HBA_PxCMD_CR) continue;
        break;
    }
    // port->is = 0;
    // port->ie = 0;
}

// AHCI port memory space initialization
void port_rebase(HBA_PORT *port) {
    // HBA->ghc=(u32)(1<<31);
    // HBA->ghc=(u32)(1<<0);
    // HBA->ghc=(u32)(1<<31);
    // HBA->ghc=(u32)(1<<1);

    stop_cmd(port); // Stop command engine
    port->serr = (u32)-1;
    port->is = 0;
    port->ie = 0;
    // Command header entry size = 32 bytes
    // Command list entry maxim count = 32 Command header
    // Command list maxim size = 32*32 = 1K per port
    u32 CL_BASE = phy_kmalloc(sizeof(HBA_CMD_HEADER) * 32); // phy_add
    memset((void *)(CL_BASE), 0, 1024);
    port->clb = CL_BASE;
    port->clbu = 0;

    // FIS entry size = 256 bytes per port
    u32 FIS_BASE = phy_kmalloc(256);
    memset((void *)(FIS_BASE), 0, 256);
    port->fb = FIS_BASE;
    port->fbu = 0;

    // Command table size = 256*32 = 8K per port
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(port->clb);
    u32 CT_BASE[32];

    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8; // 8 prdt entries per command table
                                // 256 bytes per command table, 64+16+48+16*8
        CT_BASE[i] = phy_kmalloc(sizeof(HBA_CMD_TBL));
        cmdheader[i].ctba = CT_BASE[i];
        cmdheader[i].ctbau = 0;
        memset((void *)(cmdheader[i].ctba), 0, sizeof(HBA_CMD_TBL));
    }
    port->is = (u32)-1;
    port->ie = (u32)-1;
    start_cmd(port); // Start command engine
}

u32 identity_SATA(HBA_PORT *port, u8 *buf) {
    // u8* buf=(do_kmalloc(1024));
    port->is = (u32)-1; // Clear pending interrupt bits
    int spin = 0;       // Spin lock timeout counter
    int slot = find_cmdslot(port);
    if (slot == -1) return 0;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(port->clb);
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(u32); // Command FIS size
    cmdheader->w = 0;                                   // Read from device
    cmdheader->c = 1;
    cmdheader->prdbc = 0;

    cmdheader->prdtl = 1; // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
    cmdtbl->prdt_entry[0].dba = (u32)(buf);
    cmdtbl->prdt_entry[0].dbc =
        512 - 1; // this value should always be set to 1 less than the actual value
    cmdtbl->prdt_entry[0].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command
    cmdfis->command = ATA_CMD_IDENTIFY;

    // The below loop waits until the port is no longer busy before issuing a
    // new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) { spin++; }
    if (spin == 1000000) {
        lprintf("Port is hung\n");
        return FALSE;
    }

    port->ci = 1 << slot; // Issue command

    // Wait for completion
    while (1) {
        // In some longer duration reads, it may be helpful to spin on the DPS
        // bit in the PxIS port field as well (1 << 5)
        if (((port->ci & (1 << slot)) == 0) && (cmdheader->prdbc >= 512)) { break; }
    }

    return 1;
}

int sata_read(int dev, u16 *buf, u64 sect, u32 count) {
    HBA_PORT *port = &(HBA->ports[dev]);

    port->is = (u32)-1; // Clear pending interrupt bits
    int spin = 0;       // Spin lock timeout counter
    int slot = find_cmdslot(port);
    if (slot == -1) return 0;
    // u64	sect_nr = (pos >> SECTOR_SIZE_SHIFT);
    // u32 n=(p->msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;// by qianglong
    // 2022.4.26

    u64 sect_nr = sect;
    // u64	sect_nr	= 3000;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(port->clb);

    cmdheader += slot;
    memset(cmdheader, 0, sizeof(HBA_CMD_HEADER));
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(u32); // Command FIS size
    // cmdheader->w = (p->msg->type == DEV_READ) ? 0 : 1;		// 0:device to
    // host,read ;1:host to device,write
    cmdheader->w = 0;
    cmdheader->c = 0;
    cmdheader->p = 1; // Software shall not set CH(pFreeSlot).P when building
                      // queued ATA commands.
    cmdheader->prdbc = 0;
    cmdheader->prdtl = 1; // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    int i = 0;

    cmdtbl->prdt_entry[i].dba = (u32)(buf);
    cmdtbl->prdt_entry[i].dbc = (count << 9) - 1; // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command

    // cmdfis->command = (p->msg->type == DEV_READ) ? ATA_CMD_READ_DMA_EX :
    // ATA_CMD_WRITE_DMA_EX;
    cmdfis->command = ATA_CMD_READ_DMA_EX;

    cmdfis->lba0 = sect_nr & 0xFF;
    cmdfis->lba1 = (sect_nr >> 8) & 0xFF;
    cmdfis->lba2 = (sect_nr >> 16) & 0xFF;
    cmdfis->device = 1 << 6; // LBA mode

    cmdfis->lba3 = (sect_nr >> 24) & 0xFF;
    cmdfis->lba4 = (sect_nr >> 32) & 0xFF;
    cmdfis->lba5 = (sect_nr >> 40) & 0xFF;

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    // The below loop waits until the port is no longer busy before issuing a
    // new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) { spin++; }
    if (spin == 1000000) {
        lprintf("\nPort is hung");
        return FALSE;
    }

    // Issue command
    port->ci = 1 << slot;

    while (1) {
        // In some longer duration reads, it may be helpful to spin on the DPS
        // bit in the PxIS port field as well (1 << 5)
        if ((port->ci & (1 << slot)) == 0) break;

        // Check again
        if (port->is & HBA_PxIS_TFES) {
            lprintf("Read disk error\n");
            return FALSE;
        }
    }
    return 1;
}

// Find a free command list slot
int find_cmdslot(HBA_PORT *port) {
    // If not set in SACT and CI, the slot is free
    u32 slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0) return i;
        slots >>= 1;
    }
    lprintf("Cannot find free command list entry\n");
    return -1;
}
void tf_err_rec(HBA_PORT *port) { // task erro recovry
    stop_cmd(port);
    start_cmd(port);
}
