
#include <mbr/ahci.h>
#include <mbr/disk.h>
#include <mbr/loaderprint.h>
#include <mbr/paging.h>
#include <mbr/string.h>
#include <mbr/x86.h>

PUBLIC volatile int sata_wait_flag = 1;

PUBLIC HBA_MEM *HBA;

// 可能存在多个AHCI控制器，为了方便目前只支持1个，这里只用遍历到的第一个。
PUBLIC AHCI_INFO ahci_info[MAX_AHCI_NUM];

PRIVATE int check_type(HBA_PORT *port);
PRIVATE void probe_port(HBA_MEM *abar);
// PRIVATE void sata_handler(int irq);
void port_rebase(HBA_PORT *port);

// 遍历pci设备，找到AHCI  by qianglong	2022.5.17
//! TODO: 按照 kernel 的实现重写
PUBLIC int ahci_sata_init() {
    lprintf("AHCI init start\n");
    // disp_int(16);
    // disp_int(32);
    // while(1);
    u32 bus, dev, fun, err_temp, sata_irq;
    memset(ahci_info, 0, sizeof(AHCI_INFO));
    u32 AHCI_cnt = 0;
    for (bus = 0; bus <= MAXBUS; bus++) {
        for (dev = 0; dev <= MAXDEV; dev++) {
            for (fun = 0; fun <= MAXFUN; fun++) {
                // 0号寄存器为device——id和vendor——id
                out_dword(PCI_CONFIG_ADDR, mk_pci_add(bus, dev, fun, 0));
                // 如果vendorid合法,表示该位置存在设备
                if ((in_dword(PCI_CONFIG_DATA) | 0xffff) != 0xffff) {
                    // 2号寄存器为classcode,设备类别
                    out_dword(0xcf8, mk_pci_add(bus, dev, fun, 2));
                    if ((in_dword(PCI_CONFIG_DATA) >> 16) ==
                        0x0106) { // 判断是否为AHCI
                        AHCI_cnt += 1;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.bus = bus;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.dev = dev;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.fun = fun;
                        out_dword(
                            PCI_CONFIG_ADDR,
                            mk_pci_add(bus, dev, fun, 9)); // 9号寄存器为ABAR
                        ahci_info[AHCI_cnt - 1].ABAR =
                            in_dword(PCI_CONFIG_DATA);
                        lprintf("ABAR: %d ", ahci_info[AHCI_cnt - 1].ABAR);
                        out_dword(PCI_CONFIG_ADDR,
                                  mk_pci_add(bus, dev, fun, 15));
                        ahci_info[AHCI_cnt - 1].irq_info =
                            in_dword(PCI_CONFIG_DATA) & 0xffff;
                        lprintf("interrupt: %d",
                                ahci_info[AHCI_cnt - 1].irq_info);
                        found_sata_dev = true;
                    }
                    // if ((in_dword(PCI_CONFIG_DATA)>>16)==0x0101)
                    // {
                    // 	lprintf(" idecontorler ");
                    // }
                }
            }
        }
    }
    // finish:
    lprintf("finish AHCI INIT \n");

    if (AHCI_cnt == 0) {
        lprintf("no AHCI HBA!");
        return FALSE;
    }

    lin_mapping_phy(rcr3(), ahci_info[0].ABAR, ahci_info[0].ABAR,
                    PG_P | PG_USS | PG_RWW);

    HBA = (HBA_MEM *)(ahci_info[0].ABAR);

    ahci_info[0].is_ahci_mode = (HBA->ghc) >> 31;

    if (ahci_info[0].is_ahci_mode == 0) {
        lprintf("AHCI is not Enable!");
        return FALSE;
    }
    ahci_info[0].portnum = (HBA->cap) & 0xff;

    probe_port(HBA);
    // lprintf("\nprobe_port");
    // disp_int(HBA->pi);
    unsigned int i = 0;
    for (i = 0; i < ahci_info[0].satadrv_num; i++) {
        port_rebase(&(HBA->ports[ahci_info[0].satadrv_atport[i]]));
    }

    return TRUE;
}

// Detect attached SATA devices
PRIVATE void probe_port(HBA_MEM *abar) {
    // Search disk in implemented ports
    u32 pi = abar->pi;
    ahci_info[0].satadrv_num = 0;
    int i = 0;
    while (i < 30) {
        if (pi & 1) {
            int dt = check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                lprintf("\nSATA drive found at port :%d \n", i);
                ahci_info[0].satadrv_num += 1;
                ahci_info[0].satadrv_atport[ahci_info[0].satadrv_num - 1] = i;
            } else if (dt == AHCI_DEV_SATAPI) {
                lprintf("\nSATAPI drive found at port : %d ", i);
            } else if (dt == AHCI_DEV_SEMB) {
                lprintf("\nSEMB drive found at port: %d ", i);
            } else if (dt == AHCI_DEV_PM) {
                lprintf("\nPM drive found at port: %d ", i);
            }
        }

        pi >>= 1;
        i++;
    }
}

// Check device type
PRIVATE int check_type(HBA_PORT *port) {
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
    // lprintf("\nCL_BASE:");disp_int(CL_BASE);
    memset((void *)(CL_BASE), 0, 1024);
    port->clb = CL_BASE;
    port->clbu = 0;

    // FIS entry size = 256 bytes per port
    u32 FIS_BASE = phy_kmalloc(256);
    // lprintf("\nFIS_BASE:");disp_int(FIS_BASE);
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
        // lprintf("\nCT_BASE:");disp_int(CT_BASE[i]);
        cmdheader[i].ctba = CT_BASE[i];
        cmdheader[i].ctbau = 0;
        memset((void *)(cmdheader[i].ctba), 0, sizeof(HBA_CMD_TBL));
    }
    port->is = (u32)-1;
    port->ie = (u32)-1;
    start_cmd(port); // Start command engine
}

PUBLIC u32 identity_SATA(HBA_PORT *port, u8 *buf) {
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
    memset(
        cmdtbl, 0,
        sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
    cmdtbl->prdt_entry[0].dba = (u32)(buf);
    cmdtbl->prdt_entry[0].dbc =
        512 -
        1; // this value should always be set to 1 less than the actual value
    cmdtbl->prdt_entry[0].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command
    cmdfis->command = ATA_CMD_IDENTIFY;

    // The below loop waits until the port is no longer busy before issuing a
    // new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        lprintf("Port is hung\n");
        return FALSE;
    }

    port->ci = 1 << slot; // Issue command

    // Wait for completion
    while (1) {
        // In some longer duration reads, it may be helpful to spin on the DPS
        // bit in the PxIS port field as well (1 << 5)
        if (((port->ci & (1 << slot)) == 0) && (cmdheader->prdbc >= 512)) {
            break;
        }
    }

    return 1;
}

int sata_read(int dev, u16 *buf, u64 sect, u32 count) {
    HBA_PORT *port = &(HBA->ports[dev]);

    port->is = (u32)-1; // Clear pending interrupt bits
    // lprintf("\nport_is:");
    // disp_int(port->is);
    int spin = 0; // Spin lock timeout counter
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
    // disp_int(cmdheader->w);
    cmdheader->c = 0;
    cmdheader->p = 1; // Software shall not set CH(pFreeSlot).P when building
                      // queued ATA commands.
    cmdheader->prdbc = 0;
    cmdheader->prdtl = 1; // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(cmdheader->ctba);
    memset(
        cmdtbl, 0,
        sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

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
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
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
PUBLIC int find_cmdslot(HBA_PORT *port) {
    // If not set in SACT and CI, the slot is free
    u32 slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0) return i;
        slots >>= 1;
    }
    lprintf("Cannot find free command list entry\n");
    return -1;
}
PUBLIC void tf_err_rec(HBA_PORT *port) { // task erro recovry
    stop_cmd(port);
    start_cmd(port);
}
