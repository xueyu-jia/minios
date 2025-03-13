#include <driver/ata/ahci.h>
#include <minios/console.h>
#include <minios/page.h>
#include <minios/memman.h>
#include <minios/protect.h>
#include <minios/kstate.h>
#include <minios/sched.h>
#include <minios/proc.h>
#include <minios/layout.h>
#include <minios/interrupt.h>
#include <minios/assert.h>
#include <minios/ioremap.h>
#include <driver/pci/pci.h>
#include <string.h>

#define PCI_SUBCLS_SATA_CTRL 0x06

// 0: free;  1: waiting
volatile int sata_wait_flag = 1;
// 0 : no error;  1: error
volatile int sata_error_flag = 0;

AHCI_INFO ahci_info[MAX_AHCI_NUM];

static int get_dev_type(int nr_ahci, int nr_port) {
    const HBA_PORT *port = &ahci_info[nr_ahci].hba_mmio->ports[nr_port];
    const u32 ssts = port->ssts;
    const u8 ipm = (ssts >> 8) & 0x0f;
    const u8 det = ssts & 0x0f;

    if (det != HBA_PORT_DET_PRESENT) { return AHCI_DEV_NULL; }
    if (ipm != HBA_PORT_IPM_ACTIVE) { return AHCI_DEV_NULL; }

    switch (port->sig) {
        case SATA_SIG_ATA: {
            return AHCI_DEV_SATA;
        } break;
        case SATA_SIG_ATAPI: {
            return AHCI_DEV_SATAPI;
        } break;
        case SATA_SIG_SEMB: {
            return AHCI_DEV_SEMB;
        } break;
        case SATA_SIG_PM: {
            return AHCI_DEV_PM;
        } break;
        default: {
            kprintf("error: unknown device signature at ahci port %d\n", nr_port);
            unreachable();
        } break;
    }
}

/*!
 * \brief probe devices attached to first ahci controller
 */
static void probe_port(int nr_ahci) {
    AHCI_INFO *info = &ahci_info[nr_ahci];
    HBA_MEM *hba = info->hba_mmio;
    info->nr_ports = hba->cap & 0xff;
    info->total_sata_drv = 0;
    const u32 port_bitmap = hba->pi;
    const int nr_ports = info->nr_ports;
    for (int port = 0; port < nr_ports; ++port) {
        if ((port_bitmap & BIT(port)) == 0) { continue; }
        switch (get_dev_type(nr_ahci, port)) {
            case AHCI_DEV_SATA: {
                info->sata_drv_port[info->total_sata_drv++] = port;
            } break;
            case AHCI_DEV_SATAPI:
                FALLTHROUGH;
            case AHCI_DEV_SEMB:
                FALLTHROUGH;
            case AHCI_DEV_PM: {
            } break;
        }
    }
}

/*!
 * \brief start command engine
 */
void start_cmd(HBA_PORT *port) {
    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR) {}

    // Set FRE (bit4) and ST (bit0)
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

/*!
 * \brief stop command engine
 */
void stop_cmd(HBA_PORT *port) {
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (true) {
        if (port->cmd & HBA_PxCMD_FR) { continue; }
        if (port->cmd & HBA_PxCMD_CR) { continue; }
        break;
    }
    // port->is = 0;
    // port->ie = 0;
}

/*!
 * \brief find a free command slot
 */
int ahci_find_cmd_slot(HBA_PORT *port) {
    // If not set in SACT and CI, the slot is free
    u32 slots = port->sact | port->ci;
    for (int i = 0; i < 32; ++i) {
        if ((slots & BIT(i)) == 0) { return i; }
    }
    kprintf("error: no available ahci cmd slot\n");
    return -1;
}

/*!
 * \brief recover from ahci hd error
 */
void tf_err_rec(HBA_PORT *port) {
    stop_cmd(port);
    start_cmd(port);
}

/*!
 * \brief initialize ahci port
 */
void port_rebase(HBA_PORT *port) {
    stop_cmd(port);

    port->serr = 0xffffffff;
    port->is = 0;
    port->ie = 0;

    // Command header entry size = 32 bytes
    // Command list entry maxim count = 32 Command header
    // Command list maxim size = 32*32 = 1K per port
    u32 CL_BASE = phy_kmalloc(sizeof(HBA_CMD_HEADER) * 32);
    memset(K_PHY2LIN(CL_BASE), 0, SZ_1K);
    port->clb = CL_BASE;
    port->clbu = 0;

    // FIS entry size = 256 bytes per port
    u32 FIS_BASE = phy_kmalloc(256);
    memset(K_PHY2LIN(FIS_BASE), 0, 256);
    port->fb = FIS_BASE;
    port->fbu = 0;

    // Command table size = 256*32 = 8K per port
    HBA_CMD_HEADER *cmdheader = K_PHY2LIN(port->clb);
    for (int i = 0; i < 32; ++i) {
        // 8 prdt entries per command table
        // 256 bytes per command table, 64+16+48+16*8
        phyaddr_t ctba = phy_kmalloc(sizeof(HBA_CMD_TBL));
        memset(K_PHY2LIN(ctba), 0, sizeof(HBA_CMD_TBL));
        cmdheader[i].prdtl = 8;
        cmdheader[i].ctba = ctba;
        cmdheader[i].ctbau = 0;
    }

    port->is = 0xffffffff;
    port->ie = 0xffffffff;

    start_cmd(port);
}

static void sata_handler(int irq) {
    UNUSED(irq);

    // 只支持一个 AHCI 控制器 ahci_info[0]，错误处理未完成
    // 因为 hd_service 是单线程的，因此只支持一个端口的中断处理
    // 必须先完成端口的清中断，再完成控制器的清中断
    // 目前没加内存屏障，有出现 bug 的可能性

    const u32 port_bitmap = ahci_hba()->is;
    if (port_bitmap == 0) {
        kprintf("error: sata_handler: interrupt on unknown port\n");
        return;
    }

    const int nr_ports = ahci_active()->nr_ports;
    int nr_port = 0;
    while (nr_port < nr_ports) {
        if (port_bitmap & BIT(nr_port)) { break; }
        ++nr_port;
    }
    assert(nr_port < nr_ports);

    //! NOTE: write BIT(i) will clear the interrupt status at BIT(i)

    const u32 status = ahci_hba()->ports[nr_port].is;

    ahci_hba()->ports[nr_port].is = status;
    assert(ahci_hba()->ports[nr_port].is == 0);

    if (unlikely(status == 0)) {
    } else if (likely(status & HBA_Port_COMPLETED)) {
        sata_error_flag = 0;
        if (kstate_on_init) {
            sata_wait_flag = 0;
        } else {
            wakeup((void *)&sata_wait_flag);
        }
    } else if (status & HBA_Port_ERROR) {
        tf_err_rec(&ahci_hba()->ports[nr_port]);
        sata_error_flag = 1;
        if (kstate_on_init) {
            sata_wait_flag = 0;
        } else {
            wakeup((void *)&sata_wait_flag);
        }
    } else {
        kprintf("error: sata_handler: unknown error %#x at port %d\n", status, nr_port);
        sata_error_flag = 1;
    }

    ahci_hba()->is = port_bitmap;
    assert(ahci_hba()->is == 0);
}

static bool do_pci_ahci_init(pci_device_info_t *info, void *arg) {
    int *total_found = (int *)arg;
    if (*total_found >= MAX_AHCI_NUM) { return false; }

    const bool is_ahci_sata =
        info->class_code == PCI_CLASS_MASS_STORAGE_CTRL && info->subclass == PCI_SUBCLS_SATA_CTRL;
    if (!is_ahci_sata) { return true; }

    const int index = (*total_found)++;
    ahci_info[index].pci_id = info->id;
    ahci_info[index].int_line = pci_io_r8(info->id, PCI_CONFIG_HDR0_INTERRUPT_LINE);
    ahci_info[index].int_pin = pci_io_r8(info->id, PCI_CONFIG_HDR0_INTERRUPT_PIN);
    ahci_info[index].hba_phy = pci_io_r32(info->id, PCI_CONFIG_HDR0_BAR5);
    ahci_info[index].hba_mmio = NULL;

    return true;
}

int ahci_sata_init() {
    memset(ahci_info, 0, sizeof(ahci_info));

    u32 ahci_cnt = 0;
    pci_visit(do_pci_ahci_init, &ahci_cnt);

    if (ahci_cnt == 0) {
        kprintf("ahci controller not fonud\n");
        return false;
    }

    //! ATTENTION: we'll always use the first ahci ctrl

    ahci_active()->hba_mmio = ioremap(ahci_active()->hba_phy, sizeof(HBA_MEM));
    assert(ahci_active()->hba_mmio != NULL);

    ahci_active()->is_ahci_mode = ahci_hba()->ghc >> 31;
    if (ahci_active()->is_ahci_mode == 0) {
        kprintf("error: ahci controller not support ahci mode\n");
        return false;
    }

    probe_port(0);
    for (size_t i = 0; i < ahci_active()->total_sata_drv; ++i) {
        port_rebase(&ahci_hba()->ports[ahci_active()->sata_drv_port[i]]);
    }

    //! enable interrupt
    ahci_hba()->ghc |= BIT(1);
    const int sata_irq = ahci_active()->int_line;
    if (sata_irq < 16) {
        put_irq_handler(sata_irq, sata_handler);
        enable_irq(sata_irq);
    } else {
        kprintf("error: found sata irq ge 16");
        return false;
    }

    return true;
}

bool identity_sata(HBA_PORT *port, u8 *buf) {
    port->is = 0xffffffff;

    const int slot = ahci_find_cmd_slot(port);
    if (slot == -1) { return false; }

    HBA_CMD_HEADER *cmd_hdr = K_PHY2LIN(port->clb);
    cmd_hdr += slot;
    // Command FIS size
    cmd_hdr->cfl = sizeof(FIS_REG_H2D) / sizeof(u32);
    // Read from device
    cmd_hdr->w = 0;
    cmd_hdr->c = 1;
    cmd_hdr->prdbc = 0;
    // PRDT entries count
    cmd_hdr->prdtl = 1;

    HBA_CMD_TBL *cmd_tbl = K_PHY2LIN(cmd_hdr->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL) + (cmd_hdr->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
    cmd_tbl->prdt_entry[0].dba = (u32)K_LIN2PHY(buf);
    // this value should always be set to 1 less than the actual value
    cmd_tbl->prdt_entry[0].dbc = 512 - 1;
    cmd_tbl->prdt_entry[0].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)cmd_tbl->cfis;
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    // Command
    cmdfis->c = 1;
    cmdfis->command = ATA_CMD_IDENTIFY;

    // The below loop waits until the port is no longer busy before issuing a new command
    const int max_spin = 1000000;
    int spin_cntr = 0;
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin_cntr < max_spin) { ++spin_cntr; }
    if (spin_cntr == max_spin) {
        kprintf("warn: identity_sata: port is suspend\n");
        return false;
    }

    if (kstate_on_init) {
        // Issue command
        port->ci = 1 << slot;
        while (sata_wait_flag) {}
        sata_wait_flag = 1;
    } else {
        disable_int_begin();
        // Issue command
        port->ci = 1 << slot;
        wait_event((void *)&sata_wait_flag);
        disable_int_end();
    }

    return true;
}
